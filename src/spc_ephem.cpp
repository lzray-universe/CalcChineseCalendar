#include "lunar/spc_ephem.hpp"
#include "lunar/frames.hpp"

#include<array>
#include<algorithm>
#include<cstdint>
#include<cmath>
#include<cstring>
#include<filesystem>
#include<fstream>
#include<limits>
#include<stdexcept>
#include<unordered_map>
#include<unordered_set>
#include<vector>

#if LUNAR_ENABLE_SERIES_FALLBACK
#include "elpmpp02/elpmpp02.hpp"
#include "vsop87a/vsop87a.hpp"
#endif

namespace fs=std::filesystem;

namespace{

struct State6{
	double px=0.0;
	double py=0.0;
	double pz=0.0;
	double vx=0.0;
	double vy=0.0;
	double vz=0.0;
};

struct StateAu{
	double px=0.0;
	double py=0.0;
	double pz=0.0;
	double vx=0.0;
	double vy=0.0;
	double vz=0.0;
};

State6 operator+(const State6&a,const State6&b){
	State6 out;
	out.px=a.px+b.px;
	out.py=a.py+b.py;
	out.pz=a.pz+b.pz;
	out.vx=a.vx+b.vx;
	out.vy=a.vy+b.vy;
	out.vz=a.vz+b.vz;
	return out;
}

State6 operator-(const State6&a,const State6&b){
	State6 out;
	out.px=a.px-b.px;
	out.py=a.py-b.py;
	out.pz=a.pz-b.pz;
	out.vx=a.vx-b.vx;
	out.vy=a.vy-b.vy;
	out.vz=a.vz-b.vz;
	return out;
}

StateAu operator+(const StateAu&a,const StateAu&b){
	StateAu out;
	out.px=a.px+b.px;
	out.py=a.py+b.py;
	out.pz=a.pz+b.pz;
	out.vx=a.vx+b.vx;
	out.vy=a.vy+b.vy;
	out.vz=a.vz+b.vz;
	return out;
}

StateAu operator-(const StateAu&a,const StateAu&b){
	StateAu out;
	out.px=a.px-b.px;
	out.py=a.py-b.py;
	out.pz=a.pz-b.pz;
	out.vx=a.vx-b.vx;
	out.vy=a.vy-b.vy;
	out.vz=a.vz-b.vz;
	return out;
}

struct SpkSeg{
	double et_begin=0.0;
	double et_end=0.0;
	int target=0;
	int center=0;
	int frame=0;
	int type=0;
	int begin_addr=0;
	int end_addr=0;
};

struct GenericMeta{
	int conbas=0;
	int ncon=0;
	int rdrbas=0;
	int nrdr=0;
	int rdrtyp=0;
	int refbas=0;
	int nref=0;
	int pdrbas=0;
	int npdr=0;
	int pdrtyp=0;
	int pktbas=0;
	int npkt=0;
	int rsvbas=0;
	int nrsv=0;
	int pktsz=0;
	int pkto=0;
	int nmeta=0;
};

std::uint32_t rd_u32_le(const std::uint8_t*p){
	return (static_cast<std::uint32_t>(p[0]))|
		   (static_cast<std::uint32_t>(p[1])<<8)|
		   (static_cast<std::uint32_t>(p[2])<<16)|
		   (static_cast<std::uint32_t>(p[3])<<24);
}

std::uint32_t rd_u32_be(const std::uint8_t*p){
	return (static_cast<std::uint32_t>(p[3]))|
		   (static_cast<std::uint32_t>(p[2])<<8)|
		   (static_cast<std::uint32_t>(p[1])<<16)|
		   (static_cast<std::uint32_t>(p[0])<<24);
}

std::uint64_t rd_u64_le(const std::uint8_t*p){
	return (static_cast<std::uint64_t>(p[0]))|
		   (static_cast<std::uint64_t>(p[1])<<8)|
		   (static_cast<std::uint64_t>(p[2])<<16)|
		   (static_cast<std::uint64_t>(p[3])<<24)|
		   (static_cast<std::uint64_t>(p[4])<<32)|
		   (static_cast<std::uint64_t>(p[5])<<40)|
		   (static_cast<std::uint64_t>(p[6])<<48)|
		   (static_cast<std::uint64_t>(p[7])<<56);
}

std::uint64_t rd_u64_be(const std::uint8_t*p){
	return (static_cast<std::uint64_t>(p[7]))|
		   (static_cast<std::uint64_t>(p[6])<<8)|
		   (static_cast<std::uint64_t>(p[5])<<16)|
		   (static_cast<std::uint64_t>(p[4])<<24)|
		   (static_cast<std::uint64_t>(p[3])<<32)|
		   (static_cast<std::uint64_t>(p[2])<<40)|
		   (static_cast<std::uint64_t>(p[1])<<48)|
		   (static_cast<std::uint64_t>(p[0])<<56);
}

std::int32_t rd_i32(const std::uint8_t*p,bool little){
	std::uint32_t v=little?rd_u32_le(p):rd_u32_be(p);
	return static_cast<std::int32_t>(v);
}

double rd_f64(const std::uint8_t*p,bool little){
	std::uint64_t bits=little?rd_u64_le(p):rd_u64_be(p);
	double out=0.0;
	std::memcpy(&out,&bits,sizeof(double));
	return out;
}

int d_to_iword(double v,const std::string&tag){
	if(!std::isfinite(v)){
		throw std::runtime_error("invalid non-finite DAF word for "+tag);
	}
	double rounded=std::round(v);
	if(std::fabs(v-rounded)>1e-10){
		throw std::runtime_error("invalid non-integer DAF word for "+tag);
	}
	if(rounded<std::numeric_limits<int>::min()||
	   rounded>std::numeric_limits<int>::max()){
		throw std::runtime_error("out-of-range DAF integer word for "+tag);
	}
	return static_cast<int>(rounded);
}

void merge_cover(std::vector<std::pair<double,double>>&items){
	std::sort(items.begin(),items.end(),[](const auto&a,const auto&b){
		if(a.first!=b.first){
			return a.first<b.first;
		}
		return a.second<b.second;
	});
	std::vector<std::pair<double,double>> merged;
	merged.reserve(items.size());
	for(const auto&it : items){
		if(merged.empty()||it.first>merged.back().second){
			merged.push_back(it);
			continue;
		}
		if(it.second>merged.back().second){
			merged.back().second=it.second;
		}
	}
	items.swap(merged);
}

void chb_val(const double*cp,int degp,double mid,double radius,double x,
			 double&p){
	double s=(x-mid)/radius;
	double s2=s*2.0;
	int j=degp+1;
	double w0=0.0;
	double w1=0.0;
	double w2=0.0;
	while(j>1){
		w2=w1;
		w1=w0;
		w0=cp[j-1]+(s2*w1-w2);
		--j;
	}
	p=s*w0-w1+cp[0];
}

void chb_int(const double*cp,int degp,double mid,double radius,double x,
			 double&p,double&dpdx){
	double s=(x-mid)/radius;
	double s2=s*2.0;
	int j=degp+1;
	double w0=0.0;
	double w1=0.0;
	double w2=0.0;
	double dw0=0.0;
	double dw1=0.0;
	double dw2=0.0;
	while(j>1){
		w2=w1;
		w1=w0;
		w0=cp[j-1]+(s2*w1-w2);

		dw2=dw1;
		dw1=dw0;
		dw0=w1*2.0+dw1*s2-dw2;
		--j;
	}
	p=cp[0]+(s*w0-w1);
	dpdx=(w0+s*dw0-dw1)/radius;
}

double lgrint_eval(int n,const double*xvals,const double*yvals,double x){
	if(n<1){
		throw std::runtime_error("invalid interpolation size");
	}
	std::vector<double> work(static_cast<std::size_t>(n));
	for(int i=0;i<n;++i){
		work[static_cast<std::size_t>(i)]=yvals[i];
	}
	for(int j=1;j<n;++j){
		for(int i=0;i<n-j;++i){
			double denom=xvals[i]-xvals[i+j];
			if(denom==0.0){
				throw std::runtime_error("invalid interpolation abscissa");
			}
			double c1=x-xvals[i+j];
			double c2=xvals[i]-x;
			work[static_cast<std::size_t>(i)]=
				(c1*work[static_cast<std::size_t>(i)]+
				 c2*work[static_cast<std::size_t>(i+1)])/denom;
		}
	}
	return work[0];
}

double lgresp_eval(int n,double first,double step,const double*yvals,double x){
	if(n<1){
		throw std::runtime_error("invalid interpolation size");
	}
	if(step==0.0){
		throw std::runtime_error("invalid interpolation step");
	}
	double newx=(x-first)/step+1.0;
	std::vector<double> work(static_cast<std::size_t>(n));
	for(int i=0;i<n;++i){
		work[static_cast<std::size_t>(i)]=yvals[i];
	}
	for(int j=1;j<n;++j){
		for(int i=0;i<n-j;++i){
			double c1=static_cast<double>(i+j+1)-newx;
			double c2=newx-static_cast<double>(i+1);
			work[static_cast<std::size_t>(i)]=
				(c1*work[static_cast<std::size_t>(i)]+
				 c2*work[static_cast<std::size_t>(i+1)])/
				static_cast<double>(j);
		}
	}
	return work[0];
}

void hrmint_eval(int n,const double*xvals,const double*yvals,double x,
				 double&f,double&df){
	if(n<1){
		throw std::runtime_error("invalid interpolation size");
	}
	int m=n*2;
	std::vector<double> col_f(static_cast<std::size_t>(m));
	std::vector<double> col_d(static_cast<std::size_t>(m),0.0);
	for(int i=0;i<m;++i){
		col_f[static_cast<std::size_t>(i)]=yvals[i];
	}

	for(int i=1;i<=n-1;++i){
		double c1=xvals[i]-x;
		double c2=x-xvals[i-1];
		double denom=xvals[i]-xvals[i-1];
		if(denom==0.0){
			throw std::runtime_error("invalid interpolation abscissa");
		}
		int prev=i*2-2;
		int this_i=prev+1;
		int next=this_i+1;

		col_d[static_cast<std::size_t>(prev)]=
			col_f[static_cast<std::size_t>(this_i)];
		col_d[static_cast<std::size_t>(this_i)]=
			(col_f[static_cast<std::size_t>(next)]-
			 col_f[static_cast<std::size_t>(prev)])/
			denom;

		double temp=col_f[static_cast<std::size_t>(this_i)]*(x-xvals[i-1])+
					col_f[static_cast<std::size_t>(prev)];
		col_f[static_cast<std::size_t>(this_i)]=
			(c1*col_f[static_cast<std::size_t>(prev)]+
			 c2*col_f[static_cast<std::size_t>(next)])/
			denom;
		col_f[static_cast<std::size_t>(prev)]=temp;
	}

	col_d[static_cast<std::size_t>(m-2)]=col_f[static_cast<std::size_t>(m-1)];
	col_f[static_cast<std::size_t>(m-2)]=
		col_f[static_cast<std::size_t>(m-1)]*(x-xvals[n-1])+
		col_f[static_cast<std::size_t>(m-2)];

	for(int j=2;j<=m-1;++j){
		for(int i=1;i<=m-j;++i){
			int xi=(i+1)/2;
			int xij=(i+j+1)/2;
			double c1=xvals[xij-1]-x;
			double c2=x-xvals[xi-1];
			double denom=xvals[xij-1]-xvals[xi-1];
			if(denom==0.0){
				throw std::runtime_error("invalid interpolation abscissa");
			}
			std::size_t idx=static_cast<std::size_t>(i-1);
			col_d[idx]=(c1*col_d[idx]+c2*col_d[idx+1]+
						(col_f[idx+1]-col_f[idx]))/
					   denom;
			col_f[idx]=(c1*col_f[idx]+c2*col_f[idx+1])/denom;
		}
	}

	f=col_f[0];
	df=col_d[0];
}

void hrmesp_eval(int n,double first,double step,const double*yvals,double x,
				 double&f,double&df){
	if(n<1){
		throw std::runtime_error("invalid interpolation size");
	}
	if(step==0.0){
		throw std::runtime_error("invalid interpolation step");
	}
	double newx=(x-first)/step+1.0;
	int m=n*2;
	std::vector<double> col_f(static_cast<std::size_t>(m));
	std::vector<double> col_d(static_cast<std::size_t>(m),0.0);
	for(int i=0;i<m;++i){
		col_f[static_cast<std::size_t>(i)]=yvals[i];
	}
	for(int i=1;i<m;i+=2){
		col_f[static_cast<std::size_t>(i)]*=step;
	}

	for(int i=1;i<=n-1;++i){
		double c1=static_cast<double>(i+1)-newx;
		double c2=newx-static_cast<double>(i);
		int prev=i*2-2;
		int this_i=prev+1;
		int next=this_i+1;

		col_d[static_cast<std::size_t>(prev)]=
			col_f[static_cast<std::size_t>(this_i)];
		col_d[static_cast<std::size_t>(this_i)]=
			col_f[static_cast<std::size_t>(next)]-
			col_f[static_cast<std::size_t>(prev)];

		double temp=col_f[static_cast<std::size_t>(this_i)]*
					 (newx-static_cast<double>(i))+
					col_f[static_cast<std::size_t>(prev)];
		col_f[static_cast<std::size_t>(this_i)]=
			c1*col_f[static_cast<std::size_t>(prev)]+
			c2*col_f[static_cast<std::size_t>(next)];
		col_f[static_cast<std::size_t>(prev)]=temp;
	}

	col_d[static_cast<std::size_t>(m-2)]=col_f[static_cast<std::size_t>(m-1)];
	col_f[static_cast<std::size_t>(m-2)]=
		col_f[static_cast<std::size_t>(m-1)]*
			(newx-static_cast<double>(n))+
		col_f[static_cast<std::size_t>(m-2)];

	for(int j=2;j<=m-1;++j){
		for(int i=1;i<=m-j;++i){
			double xi=static_cast<double>((i+1)/2);
			double xij=static_cast<double>((i+j+1)/2);
			double c1=xij-newx;
			double c2=newx-xi;
			double denom=xij-xi;
			std::size_t idx=static_cast<std::size_t>(i-1);
			col_d[idx]=(c1*col_d[idx]+c2*col_d[idx+1]+
						(col_f[idx+1]-col_f[idx]))/
					   denom;
			col_f[idx]=(c1*col_f[idx]+c2*col_f[idx+1])/denom;
		}
	}

	f=col_f[0];
	df=col_d[0]/step;
}

void chb_igr(const double*cp,int degp,double mid,double radius,double x,
			 double&p,double&itgrl){
	if(degp<0){
		throw std::runtime_error("invalid Chebyshev degree");
	}
	if(!(radius>0.0)){
		throw std::runtime_error("invalid Chebyshev radius");
	}

	int nterms=degp+1;
	double s=(x-mid)/radius;
	double s2=s*2.0;

	double a2=(nterms>=3)?(cp[0]-cp[2]*0.5):cp[0];
	double adegp1=0.0;
	double adegp2=0.0;
	if(degp>=2){
		adegp1=cp[degp-1]*0.5/static_cast<double>(degp);
	}
	if(degp>=1){
		adegp2=cp[degp]*0.5/static_cast<double>(degp+1);
	}

	double f0=(degp==0)?a2:adegp2;
	double f1=0.0;
	double f2=0.0;

	double z0=f0;
	double z1=0.0;
	double z2=0.0;

	double w0=0.0;
	double w1=0.0;
	double w2=0.0;

	for(int i=nterms;i>1;--i){
		double ai=0.0;
		if(i==2){
			ai=a2;
		}else if(i<nterms){
			ai=(cp[i-2]-cp[i])*0.5/static_cast<double>(i-1);
		}else{
			ai=adegp1;
		}

		f2=f1;
		f1=f0;
		f0=ai+(s2*f1-f2);

		z2=z1;
		z1=z0;
		z0=ai-z2;

		w2=w1;
		w1=w0;
		w0=cp[i-1]+(s2*w1-w2);
	}

	double c0=z1;
	itgrl=radius*(c0+s*f0-f1);
	p=cp[0]+(s*w0-w1);
}

double sign_mag(double a,double b){
	return (b>=0.0)?std::fabs(a):-std::fabs(a);
}

double dot3(const std::array<double,3>&a,const std::array<double,3>&b){
	return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

double norm3(const std::array<double,3>&a){ return std::sqrt(dot3(a,a)); }

std::array<double,3> cross3(const std::array<double,3>&a,
							const std::array<double,3>&b){
	return {
		a[1]*b[2]-a[2]*b[1],
		a[2]*b[0]-a[0]*b[2],
		a[0]*b[1]-a[1]*b[0],
	};
}

std::array<double,3> lin3(double a,const std::array<double,3>&x,double b,
						  const std::array<double,3>&y){
	return {
		a*x[0]+b*y[0],
		a*x[1]+b*y[1],
		a*x[2]+b*y[2],
	};
}

bool normalize3(std::array<double,3>&v){
	double mag=norm3(v);
	if(mag==0.0){
		return false;
	}
	v[0]/=mag;
	v[1]/=mag;
	v[2]/=mag;
	return true;
}

double brcktd(double number,double end1,double end2){
	if(end1<end2){
		return std::max(end1,std::min(end2,number));
	}
	return std::max(end2,std::min(end1,number));
}

void stmp03_eval(double x,double&c0,double&c1,double&c2,double&c3){
	static const std::array<double,18> pairs=[](){
		std::array<double,18> out{};
		for(int i=1;i<=18;++i){
			out[static_cast<std::size_t>(i-1)]=
				1.0/(static_cast<double>(i)*static_cast<double>(i+1));
		}
		return out;
	}();
	static const double lbound=[](){
		double y=std::log(2.0)+std::log(std::numeric_limits<double>::max());
		return -y*y;
	}();

	if(x<=lbound){
		throw std::runtime_error("Stumpff argument out of range");
	}

	if(x<-1.0){
		double z=std::sqrt(-x);
		c0=std::cosh(z);
		c1=std::sinh(z)/z;
		c2=(1.0-c0)/x;
		c3=(1.0-c1)/x;
		return;
	}
	if(x>1.0){
		double z=std::sqrt(x);
		c0=std::cos(z);
		c1=std::sin(z)/z;
		c2=(1.0-c0)/x;
		c3=(1.0-c1)/x;
		return;
	}

	c3=1.0;
	for(int i=18;i>=4;i-=2){
		c3=1.0-x*pairs[static_cast<std::size_t>(i-1)]*c3;
	}
	c3=pairs[1]*c3;

	c2=1.0;
	for(int i=17;i>=3;i-=2){
		c2=1.0-x*pairs[static_cast<std::size_t>(i-1)]*c2;
	}
	c2=pairs[0]*c2;

	c1=1.0-x*c3;
	c0=1.0-x*c2;
}

double vsep_eval(const std::array<double,3>&v1,const std::array<double,3>&v2){
	std::array<double,3> u1=v1;
	std::array<double,3> u2=v2;
	double m1=norm3(u1);
	if(m1==0.0){
		return 0.0;
	}
	double m2=norm3(u2);
	if(m2==0.0){
		return 0.0;
	}
	u1[0]/=m1;
	u1[1]/=m1;
	u1[2]/=m1;
	u2[0]/=m2;
	u2[1]/=m2;
	u2[2]/=m2;

	double dp=dot3(u1,u2);
	if(dp>0.0){
		std::array<double,3> t{
			u1[0]-u2[0],
			u1[1]-u2[1],
			u1[2]-u2[2],
		};
		return std::asin(norm3(t)*0.5)*2.0;
	}
	if(dp<0.0){
		std::array<double,3> t{
			u1[0]+u2[0],
			u1[1]+u2[1],
			u1[2]+u2[2],
		};
		return PI-std::asin(norm3(t)*0.5)*2.0;
	}
	return PI*0.5;
}

std::array<double,3> vrotv_eval(const std::array<double,3>&v,
								const std::array<double,3>&axis,double theta){
	double amag=norm3(axis);
	if(amag==0.0){
		return v;
	}

	std::array<double,3> x{
		axis[0]/amag,
		axis[1]/amag,
		axis[2]/amag,
	};
	double proj=dot3(v,x);
	std::array<double,3> p{
		proj*x[0],
		proj*x[1],
		proj*x[2],
	};
	std::array<double,3> v1{
		v[0]-p[0],
		v[1]-p[1],
		v[2]-p[2],
	};
	std::array<double,3> v2=cross3(x,v1);
	double c=std::cos(theta);
	double s=std::sin(theta);
	std::array<double,3> rplane=lin3(c,v1,s,v2);
	return {
		rplane[0]+p[0],
		rplane[1]+p[1],
		rplane[2]+p[2],
	};
}

void prop2b_eval(double gm,const std::array<double,6>&pvinit,double dt,
				 std::array<double,6>&pvprop){
	std::array<double,3> pos{pvinit[0],pvinit[1],pvinit[2]};
	std::array<double,3> vel{pvinit[3],pvinit[4],pvinit[5]};

	if(!(gm>0.0)){
		throw std::runtime_error("non-positive gravitational parameter");
	}
	if(norm3(pos)==0.0){
		throw std::runtime_error("zero position vector in two-body propagation");
	}
	if(norm3(vel)==0.0){
		throw std::runtime_error("zero velocity vector in two-body propagation");
	}

	double r0=norm3(pos);
	double rv=dot3(pos,vel);

	std::array<double,3> hvec=cross3(pos,vel);
	double h2=dot3(hvec,hvec);
	if(h2==0.0){
		throw std::runtime_error("non-conic motion in two-body propagation");
	}

	std::array<double,3> tmp=cross3(vel,hvec);
	std::array<double,3> eqvec=lin3(1.0/gm,tmp,-1.0/r0,pos);
	double e=norm3(eqvec);
	double q=h2/(gm*(e+1.0));

	double f=1.0-e;
	double b=std::sqrt(q/gm);
	double br0=b*r0;
	double b2rv=b*b*rv;
	double bq=b*q;
	double qovr0=q/r0;

	double maxc=
		std::max({1.0,std::fabs(br0),std::fabs(b2rv),std::fabs(bq),
				  std::fabs(qovr0/bq)});

	double bound=0.0;
	if(f<0.0){
		double logmxc=std::log(maxc);
		double logdpm=std::log(std::numeric_limits<double>::max()/2.0);
		double fixed=logdpm-logmxc;
		double rootf=std::sqrt(-f);
		double logf=std::log(-f);
		bound=std::min(fixed/rootf,(fixed+1.5*logf)/rootf);
	}else{
		double logbnd=(std::log(1.5)+std::log(std::numeric_limits<double>::max())-
					   std::log(maxc))/
					  3.0;
		bound=std::exp(logbnd);
	}

	if(dt==0.0){
		pvprop=pvinit;
		return;
	}

	double x=brcktd(dt/bq,-bound,bound);
	double fx2=f*x*x;
	double c0=0.0;
	double c1=0.0;
	double c2=0.0;
	double c3=0.0;
	stmp03_eval(fx2,c0,c1,c2,c3);
	double kfun=x*(br0*c1+x*(b2rv*c2+x*(bq*c3)));

	double lower=0.0;
	double upper=0.0;
	if(dt<0.0){
		upper=0.0;
		lower=x;
		while(kfun>dt){
			upper=lower;
			lower*=2.0;
			double oldx=x;
			x=brcktd(lower,-bound,bound);
			if(x==oldx){
				fx2=f*bound*bound;
				stmp03_eval(fx2,c0,c1,c2,c3);
				double kfunl=
					-bound*(br0*c1-bound*(b2rv*c2-bound*bq*c3));
				double kfunu=
					bound*(br0*c1+bound*(b2rv*c2+bound*bq*c3));
				(void)kfunl;
				(void)kfunu;
				throw std::runtime_error("delta time out of range for two-body propagation");
			}
			fx2=f*x*x;
			stmp03_eval(fx2,c0,c1,c2,c3);
			kfun=x*(br0*c1+x*(b2rv*c2+x*(bq*c3)));
		}
	}else{
		lower=0.0;
		upper=x;
		while(kfun<dt){
			lower=upper;
			upper*=2.0;
			double oldx=x;
			x=brcktd(upper,-bound,bound);
			if(x==oldx){
				fx2=f*bound*bound;
				stmp03_eval(fx2,c0,c1,c2,c3);
				double kfunl=
					-bound*(br0*c1-bound*(b2rv*c2-bound*bq*c3));
				double kfunu=
					bound*(br0*c1+bound*(b2rv*c2+bound*bq*c3));
				(void)kfunl;
				(void)kfunu;
				throw std::runtime_error("delta time out of range for two-body propagation");
			}
			fx2=f*x*x;
			stmp03_eval(fx2,c0,c1,c2,c3);
			kfun=x*(br0*c1+x*(b2rv*c2+x*bq*c3));
		}
	}

	x=std::min(upper,std::max(lower,(lower+upper)*0.5));
	fx2=f*x*x;
	stmp03_eval(fx2,c0,c1,c2,c3);
	int lcount=0;
	int mostc=1000;
	while(x>lower&&x<upper&&lcount<mostc){
		kfun=x*(br0*c1+x*(b2rv*c2+x*bq*c3));
		if(kfun>dt){
			upper=x;
		}else if(kfun<dt){
			lower=x;
		}else{
			upper=x;
			lower=x;
		}
		if(mostc>64){
			if(upper!=0.0&&lower!=0.0){
				mostc=64;
				lcount=0;
			}
		}
		x=std::min(upper,std::max(lower,(lower+upper)*0.5));
		fx2=f*x*x;
		stmp03_eval(fx2,c0,c1,c2,c3);
		++lcount;
	}

	double x2=x*x;
	double x3=x2*x;
	double br=br0*c0+x*(b2rv*c1+x*(bq*c2));
	double pc=1.0-qovr0*x2*c2;
	double vc=dt-bq*x3*c3;
	double pcdot=-(qovr0/br)*x*c1;
	double vcdot=1.0-bq/br*x2*c2;

	std::array<double,3> pvec=lin3(pc,pos,vc,vel);
	std::array<double,3> vvec=lin3(pcdot,pos,vcdot,vel);
	pvprop={pvec[0],pvec[1],pvec[2],vvec[0],vvec[1],vvec[2]};
}

double kpsolv_eval(const std::array<double,2>&evec){
	double h=evec[0];
	double k=evec[1];
	double ecc2=h*h+k*k;
	if(ecc2>=1.0){
		throw std::runtime_error("equinoctial eccentricity vector out of range");
	}

	double y0=-h;
	double xm=0.0;
	double ecc=std::sqrt(ecc2);
	double xl=0.0;
	double xu=0.0;
	if(y0>0.0){
		xu=0.0;
		xl=-ecc;
	}else if(y0<0.0){
		xu=ecc;
		xl=0.0;
	}else{
		return 0.0;
	}

	int maxit=std::min(32,std::max(1,static_cast<int>(
										 std::llround(1.0/(1.0-ecc)))));
	for(int i=0;i<maxit;++i){
		xm=std::max(xl,std::min(xu,(xl+xu)*0.5));
		double yxm=xm-h*std::cos(xm)-k*std::sin(xm);
		if(yxm>0.0){
			xu=xm;
		}else{
			xl=xm;
		}
	}

	double x=xm;
	for(int i=0;i<5;++i){
		double cosx=std::cos(x);
		double sinx=std::sin(x);
		double yx=x-h*cosx-k*sinx;
		double ypx=h*sinx+1.0-k*cosx;
		x-=yx/ypx;
	}
	return x;
}

double kepleq_eval(double ml,double h,double k){
	double e2=h*h+k*k;
	if(e2>=0.81){
		throw std::runtime_error("equinoctial H/K values out of range");
	}
	std::array<double,2> evec{
		-h*std::cos(ml)+k*std::sin(ml),
		h*std::sin(ml)+k*std::cos(ml),
	};
	return ml+kpsolv_eval(evec);
}

void mxv3_eval(const std::array<double,9>&m,const std::array<double,3>&x,
			   std::array<double,3>&y){
	y[0]=m[0]*x[0]+m[3]*x[1]+m[6]*x[2];
	y[1]=m[1]*x[0]+m[4]*x[1]+m[7]*x[2];
	y[2]=m[2]*x[0]+m[5]*x[1]+m[8]*x[2];
}

void eqncpv_eval(double et,double epoch,const double*eqel,double rapol,
				 double decpol,std::array<double,6>&state){
	double a=eqel[0];
	double ecc=std::sqrt(eqel[1]*eqel[1]+eqel[2]*eqel[2]);
	if(a<=0.0){
		throw std::runtime_error("non-positive semi-major axis in type 17 segment");
	}
	if(ecc>0.9){
		throw std::runtime_error("eccentricity out of range in type 17 segment");
	}

	double sa=std::sin(rapol);
	double ca=std::cos(rapol);
	double sd=std::sin(decpol);
	double cd=std::cos(decpol);
	std::array<double,9> trans{
		-sa,-ca*sd,ca*cd,
		ca,-sa*sd,sa*cd,
		0.0,cd,sd,
	};

	double dt=et-epoch;
	double dlpdt=eqel[6];
	double dlp=dt*dlpdt;
	double can=std::cos(dlp);
	double san=std::sin(dlp);
	double h=eqel[1]*can+eqel[2]*san;
	double k=eqel[2]*can-eqel[1]*san;
	double l=eqel[3];

	double nodedt=eqel[8];
	double node=dt*nodedt;
	double cn=std::cos(node);
	double sn=std::sin(node);
	double p=eqel[4]*cn+eqel[5]*sn;
	double q=eqel[5]*cn-eqel[4]*sn;
	double mldt=eqel[7];
	double prate=dlpdt-nodedt;

	double b=std::sqrt(1.0-h*h-k*k);
	b=1.0/(b+1.0);

	double di=1.0/(p*p+1.0+q*q);
	std::array<double,3> vf{
		(1.0-p*p+q*q)*di,
		2.0*p*q*di,
		-2.0*p*di,
	};
	std::array<double,3> vg{
		2.0*p*q*di,
		(p*p+1.0-q*q)*di,
		2.0*q*di,
	};

	double ml=l+std::fmod(mldt*dt,TWO_PI);
	double eecan=kepleq_eval(ml,h,k);
	double sf=std::sin(eecan);
	double cf=std::cos(eecan);

	double x1=a*((1.0-b*h*h)*cf+(h*k*b*sf-k));
	double y1=a*((1.0-b*k*k)*sf+(h*k*b*cf-h));

	double rb=h*sf+k*cf;
	double r=a*(1.0-rb);
	double ra=mldt*a*a/r;

	double dx1=ra*(-sf+h*b*rb);
	double dy1=ra*(cf-k*b*rb);
	double nfac=1.0-dlpdt/mldt;
	double dx=nfac*dx1-prate*y1;
	double dy=nfac*dy1+prate*x1;

	std::array<double,3> xhold_pos=lin3(x1,vf,y1,vg);
	std::array<double,3> temp{
		-nodedt*xhold_pos[1],
		nodedt*xhold_pos[0],
		0.0,
	};
	std::array<double,3> xhold_vel{
		temp[0]+dx*vf[0]+dy*vg[0],
		temp[1]+dx*vf[1]+dy*vg[1],
		temp[2]+dx*vf[2]+dy*vg[2],
	};

	std::array<double,3> out_pos{};
	std::array<double,3> out_vel{};
	mxv3_eval(trans,xhold_pos,out_pos);
	mxv3_eval(trans,xhold_vel,out_vel);

	state={
		out_pos[0],out_pos[1],out_pos[2],
		out_vel[0],out_vel[1],out_vel[2],
	};
}

void eval_type15_record(double et,const double*rec,std::array<double,6>&state){
	double epoch=rec[0];
	std::array<double,3> tp{rec[1],rec[2],rec[3]};
	std::array<double,3> pa{rec[4],rec[5],rec[6]};
	double p=rec[7];
	double ecc=rec[8];
	int j2flg=d_to_iword(rec[9],"J2FLAG");
	std::array<double,3> pv{rec[10],rec[11],rec[12]};
	double gm=rec[13];
	double oj2=rec[14];
	double rpl=rec[15];

	if(p<=0.0){
		throw std::runtime_error("non-positive type 15 semi-latus rectum");
	}
	if(ecc<0.0){
		throw std::runtime_error("negative type 15 eccentricity");
	}
	if(gm<=0.0){
		throw std::runtime_error("non-positive type 15 central mass");
	}
	if(norm3(tp)==0.0||norm3(pa)==0.0||norm3(pv)==0.0){
		throw std::runtime_error("zero type 15 orientation vector");
	}
	if(rpl<0.0){
		throw std::runtime_error("negative type 15 central body radius");
	}

	normalize3(pa);
	normalize3(tp);
	normalize3(pv);
	double d=dot3(pa,tp);
	if(std::fabs(d)>1e-5){
		throw std::runtime_error("invalid type 15 periapsis/trajectory pole basis");
	}

	double near=p/(ecc+1.0);
	double speed=std::sqrt(gm/p)*(ecc+1.0);
	std::array<double,3> p0{
		near*pa[0],
		near*pa[1],
		near*pa[2],
	};
	std::array<double,3> v0=cross3(tp,pa);
	double v0mag=norm3(v0);
	if(v0mag==0.0){
		throw std::runtime_error("invalid type 15 periapsis velocity direction");
	}
	double scale=speed/v0mag;
	v0[0]*=scale;
	v0[1]*=scale;
	v0[2]*=scale;

	std::array<double,6> s0{
		p0[0],p0[1],p0[2],v0[0],v0[1],v0[2],
	};
	double dt=et-epoch;
	prop2b_eval(gm,s0,dt,state);

	if(j2flg!=3&&oj2!=0.0&&ecc<1.0&&near>rpl){
		double oneme2=1.0-ecc*ecc;
		double dmdt=oneme2/p*std::sqrt(gm*oneme2/p);
		double manom=dmdt*dt;

		double theta=std::fmod(manom,TWO_PI);
		if(std::fabs(theta)>PI){
			theta-=sign_mag(TWO_PI,theta);
		}
		double k2pi=manom-theta;

		std::array<double,3> pos{state[0],state[1],state[2]};
		double ta=vsep_eval(pa,pos);
		ta=sign_mag(ta,theta);
		ta+=k2pi;

		double cosinc=dot3(pv,tp);
		double z=ta*1.5*oj2*std::pow(rpl/p,2.0);
		double dnode=-z*cosinc;
		double dperi=z*(2.5*cosinc*cosinc-0.5);

		if(j2flg!=1){
			std::array<double,3> rpos=vrotv_eval(
				{state[0],state[1],state[2]},tp,dperi);
			std::array<double,3> rvel=vrotv_eval(
				{state[3],state[4],state[5]},tp,dperi);
			state[0]=rpos[0];
			state[1]=rpos[1];
			state[2]=rpos[2];
			state[3]=rvel[0];
			state[4]=rvel[1];
			state[5]=rvel[2];
		}
		if(j2flg!=2){
			std::array<double,3> rpos=vrotv_eval(
				{state[0],state[1],state[2]},pv,dnode);
			std::array<double,3> rvel=vrotv_eval(
				{state[3],state[4],state[5]},pv,dnode);
			state[0]=rpos[0];
			state[1]=rpos[1];
			state[2]=rpos[2];
			state[3]=rvel[0];
			state[4]=rvel[1];
			state[5]=rvel[2];
		}
	}
}

void eval_type17_record(double et,const double*rec,std::array<double,6>&state){
	double a=rec[1];
	double h=rec[2];
	double k=rec[3];
	double ecc=std::sqrt(h*h+k*k);
	if(a<=0.0){
		throw std::runtime_error("non-positive type 17 semi-major axis");
	}
	if(ecc>0.9){
		throw std::runtime_error("type 17 eccentricity out of range");
	}
	eqncpv_eval(et,rec[0],rec+1,rec[10],rec[11],state);
}

void eval_type5_record(double et,const std::array<double,6>&pv1,double t1,
					   const std::array<double,6>&pv2,double t2,double gm,
					   std::array<double,6>&state){
	if(t1!=t2){
		std::array<double,6> s1{};
		std::array<double,6> s2{};
		prop2b_eval(gm,pv1,et-t1,s1);
		prop2b_eval(gm,pv2,et-t2,s2);

		double numer=et-t1;
		double denom=t2-t1;
		double arg=numer*PI/denom;
		double dargdt=PI/denom;
		double w=0.5*std::cos(arg)+0.5;
		double dwdt=-0.5*std::sin(arg)*dargdt;

		for(int i=0;i<6;++i){
			state[static_cast<std::size_t>(i)]=
				w*s1[static_cast<std::size_t>(i)]+
				(1.0-w)*s2[static_cast<std::size_t>(i)];
		}
		for(int i=0;i<3;++i){
			state[static_cast<std::size_t>(i+3)]+=
				dwdt*(s1[static_cast<std::size_t>(i)]-
					  s2[static_cast<std::size_t>(i)]);
		}
	}else{
		prop2b_eval(gm,pv1,et-t1,state);
	}
}

void eval_mda(int maxdim,const double*rec,double et,State6&out){
	if(maxdim<1){
		throw std::runtime_error("invalid difference line dimension");
	}

	double tl=rec[0];
	std::vector<double> g(static_cast<std::size_t>(maxdim));
	for(int i=0;i<maxdim;++i){
		g[static_cast<std::size_t>(i)]=rec[1+i];
	}

	std::array<double,3> refpos={
		rec[maxdim+1],
		rec[maxdim+3],
		rec[maxdim+5],
	};
	std::array<double,3> refvel={
		rec[maxdim+2],
		rec[maxdim+4],
		rec[maxdim+6],
	};

	std::vector<double> dt(static_cast<std::size_t>(maxdim*3));
	for(int c=0;c<3;++c){
		for(int j=0;j<maxdim;++j){
			dt[static_cast<std::size_t>(c*maxdim+j)]=
				rec[maxdim+7+c*maxdim+j];
		}
	}

	int kqmax1=d_to_iword(rec[4*maxdim+7],"KQMAX1");
	std::array<int,3> kq={
		d_to_iword(rec[4*maxdim+8],"KQ1"),
		d_to_iword(rec[4*maxdim+9],"KQ2"),
		d_to_iword(rec[4*maxdim+10],"KQ3"),
	};
	if(kqmax1<1||kqmax1>maxdim){
		throw std::runtime_error("invalid difference line integration order");
	}
	for(int c=0;c<3;++c){
		if(kq[c]<0||kq[c]>maxdim){
			throw std::runtime_error("invalid difference line component order");
		}
	}

	double delta=et-tl;
	double tp=delta;
	int mq2=kqmax1-2;
	int ks=kqmax1-1;

	std::vector<double> fc(static_cast<std::size_t>(maxdim+1),0.0);
	std::vector<double> wc(static_cast<std::size_t>(maxdim),0.0);
	std::vector<double> w(static_cast<std::size_t>(maxdim+2),0.0);
	fc[0]=1.0;

	for(int j=1;j<=mq2;++j){
		double gj=g[static_cast<std::size_t>(j-1)];
		if(gj==0.0){
			throw std::runtime_error("invalid zero step in difference line");
		}
		fc[static_cast<std::size_t>(j)]=tp/gj;
		wc[static_cast<std::size_t>(j-1)]=delta/gj;
		tp=delta+gj;
	}

	for(int j=1;j<=kqmax1;++j){
		w[static_cast<std::size_t>(j)]=1.0/static_cast<double>(j);
	}

	int jx=0;
	int ks1=ks-1;
	while(ks>=2){
		++jx;
		for(int j=1;j<=jx;++j){
			int idx=j+ks-1;
			int prev=j+ks1-1;
			w[static_cast<std::size_t>(idx)]=
				fc[static_cast<std::size_t>(j)]*
					w[static_cast<std::size_t>(prev)]-
				wc[static_cast<std::size_t>(j-1)]*
					w[static_cast<std::size_t>(idx)];
		}
		ks=ks1;
		--ks1;
	}

	std::array<double,3> pos={0.0,0.0,0.0};
	for(int c=0;c<3;++c){
		int kqq=kq[c];
		double sum=0.0;
		for(int j=kqq;j>=1;--j){
			sum+=dt[static_cast<std::size_t>(c*maxdim+(j-1))]*
				  w[static_cast<std::size_t>(j+ks-1)];
		}
		pos[static_cast<std::size_t>(c)]=
			refpos[static_cast<std::size_t>(c)]+
			delta*(refvel[static_cast<std::size_t>(c)]+delta*sum);
	}

	for(int j=1;j<=jx;++j){
		int idx=j+ks-1;
		int prev=j+ks1-1;
		w[static_cast<std::size_t>(idx)]=
			fc[static_cast<std::size_t>(j)]*w[static_cast<std::size_t>(prev)]-
			wc[static_cast<std::size_t>(j-1)]*w[static_cast<std::size_t>(idx)];
	}
	--ks;

	std::array<double,3> vel={0.0,0.0,0.0};
	for(int c=0;c<3;++c){
		int kqq=kq[c];
		double sum=0.0;
		for(int j=kqq;j>=1;--j){
			sum+=dt[static_cast<std::size_t>(c*maxdim+(j-1))]*
				  w[static_cast<std::size_t>(j+ks-1)];
		}
		vel[static_cast<std::size_t>(c)]=
			refvel[static_cast<std::size_t>(c)]+delta*sum;
	}

	out.px=pos[0];
	out.py=pos[1];
	out.pz=pos[2];
	out.vx=vel[0];
	out.vy=vel[1];
	out.vz=vel[2];
}

std::string normalize_path(const std::string&path){
	std::error_code ec;
	fs::path p(path);
	fs::path wk=fs::weakly_canonical(p,ec);
	if(!ec){
		return wk.string();
	}
	fs::path ab=fs::absolute(p,ec);
	if(!ec){
		return ab.lexically_normal().string();
	}
	return p.lexically_normal().string();
}

std::mutex&cache_mtx(){
	static std::mutex mtx;
	return mtx;
}

std::unordered_map<std::string,std::weak_ptr<SpkFile>>&cache_map(){
	static std::unordered_map<std::string,std::weak_ptr<SpkFile>> cache;
	return cache;
}

}

struct SpkFile{
	std::string filepath;
	bool little=false;
	int nd=0;
	int ni=0;
	int first_sum_rec=0;
	int last_sum_rec=0;

	std::vector<double> words;
	std::vector<SpkSeg> segs;
	std::unordered_map<int,std::vector<std::size_t>> seg_idx;
	std::vector<int> obj_ids;
	std::unordered_map<int,std::vector<std::pair<double,double>>> cover_map;

	explicit SpkFile(const std::string&path){ load(path); }

	double word_at(int addr) const{
		if(addr<1||static_cast<std::size_t>(addr)>words.size()){
			throw std::runtime_error("DAF address out of range");
		}
		return words[static_cast<std::size_t>(addr-1)];
	}

	const double*word_ptr(int addr,int count) const{
		if(addr<1||count<0){
			throw std::runtime_error("invalid DAF address range");
		}
		std::size_t start=static_cast<std::size_t>(addr-1);
		std::size_t len=static_cast<std::size_t>(count);
		if(start+len>words.size()){
			throw std::runtime_error("DAF address span out of range");
		}
		return &words[start];
	}

	GenericMeta parse_generic_meta(const SpkSeg&seg) const{
		int metasz=d_to_iword(word_at(seg.end_addr),"NMETA");
		if(metasz<15){
			throw std::runtime_error("invalid generic segment metadata");
		}

		int ametas=metasz;
		if(metasz==15){
			++metasz;
			ametas=metasz;
		}else if(metasz>17){
			metasz=17;
		}

		int begmta=seg.end_addr-ametas+1;
		if(begmta<seg.begin_addr){
			throw std::runtime_error("invalid generic segment metadata range");
		}

		std::array<int,17> meta{};
		for(int i=0;i<metasz;++i){
			meta[static_cast<std::size_t>(i)]=d_to_iword(
				word_at(begmta+i),"GENERIC META");
		}
		meta[16]=metasz;
		for(int i=metasz;i<=16;++i){
			meta[static_cast<std::size_t>(i-1)]=0;
		}

		int begm1=seg.begin_addr-1;
		meta[0]+=begm1;
		meta[5]+=begm1;
		meta[2]+=begm1;
		meta[7]+=begm1;
		meta[10]+=begm1;
		meta[12]+=begm1;

		GenericMeta gm;
		gm.conbas=meta[0];
		gm.ncon=meta[1];
		gm.rdrbas=meta[2];
		gm.nrdr=meta[3];
		gm.rdrtyp=meta[4];
		gm.refbas=meta[5];
		gm.nref=meta[6];
		gm.pdrbas=meta[7];
		gm.npdr=meta[8];
		gm.pdrtyp=meta[9];
		gm.pktbas=meta[10];
		gm.npkt=meta[11];
		gm.rsvbas=meta[12];
		gm.nrsv=meta[13];
		gm.pktsz=meta[14];
		gm.pkto=meta[15];
		gm.nmeta=meta[16];

		return gm;
	}

	bool find_generic_ref(const GenericMeta&gm,double x,int&idx,
						  double&value) const{
		idx=0;
		value=0.0;

		if(gm.rdrtyp==0||gm.rdrtyp==1){
			if(gm.nref<2){
				throw std::runtime_error(
					"invalid implicit generic reference partition");
			}
			double ref0=word_at(gm.refbas+1);
			double step=word_at(gm.refbas+2);
			if(step==0.0){
				throw std::runtime_error("invalid implicit generic step size");
			}
			double endref=ref0+static_cast<double>(gm.npkt-1)*step;

			if(gm.rdrtyp==0){
				if(x<ref0){
					return false;
				}
				if(x>endref){
					idx=gm.npkt;
					value=endref;
					return true;
				}
				if(gm.npkt>1){
					double dptemp=(x-ref0)/step+1.0;
					idx=static_cast<int>(dptemp);
					if(idx<1){
						idx=1;
					}
					if(idx>gm.npkt){
						idx=gm.npkt;
					}
				}else{
					idx=1;
				}
				value=ref0+static_cast<double>(idx-1)*step;
				return true;
			}

			if(x<ref0){
				idx=1;
				value=ref0;
				return true;
			}
			if(x>endref){
				idx=gm.npkt;
				value=endref;
				return true;
			}
			if(gm.npkt>1){
				double dptemp=(x-ref0)/step+1.5;
				idx=static_cast<int>(dptemp);
				if(idx<1){
					idx=1;
				}
				if(idx>gm.npkt){
					idx=gm.npkt;
				}
			}else{
				idx=1;
			}
			value=ref0+static_cast<double>(idx-1)*step;
			return true;
		}

		if(gm.nref<1){
			return false;
		}
		std::vector<double> refs(static_cast<std::size_t>(gm.nref));
		for(int i=0;i<gm.nref;++i){
			refs[static_cast<std::size_t>(i)]=word_at(gm.refbas+1+i);
		}

		if(gm.rdrtyp==2){
			auto it=std::lower_bound(refs.begin(),refs.end(),x);
			int nlt=static_cast<int>(it-refs.begin());
			if(nlt<=0){
				return false;
			}
			idx=nlt;
			value=refs[static_cast<std::size_t>(idx-1)];
		}else if(gm.rdrtyp==3){
			auto it=std::upper_bound(refs.begin(),refs.end(),x);
			int nle=static_cast<int>(it-refs.begin());
			if(nle<=0){
				return false;
			}
			idx=nle;
			value=refs[static_cast<std::size_t>(idx-1)];
		}else if(gm.rdrtyp==4){
			if(x<=refs.front()){
				idx=1;
				value=refs.front();
			}else if(x>=refs.back()){
				idx=gm.nref;
				value=refs.back();
			}else{
				auto it=std::upper_bound(refs.begin(),refs.end(),x);
				int right=static_cast<int>(it-refs.begin());
				int left=right-1;
				double dr=refs[static_cast<std::size_t>(right)]-x;
				double dl=x-refs[static_cast<std::size_t>(left)];
				if(dr<=dl){
					idx=right+1;
					value=refs[static_cast<std::size_t>(right)];
				}else{
					idx=left+1;
					value=refs[static_cast<std::size_t>(left)];
				}
			}
		}else{
			throw std::runtime_error("unsupported generic reference index type");
		}

		if(idx<1||idx>gm.npkt){
			throw std::runtime_error("generic reference index out of packet range");
		}
		return true;
	}

	void fetch_generic_packet(const GenericMeta&gm,int idx,
							  std::vector<double>&packet) const{
		if(idx<1||idx>gm.npkt){
			throw std::runtime_error("generic packet index out of range");
		}

		if(gm.pdrtyp==0){
			if(gm.pktsz<1){
				throw std::runtime_error("invalid generic packet size");
			}
			int b=0;
			if(gm.pkto==0){
				b=gm.pktbas+(idx-1)*gm.pktsz+1;
			}else{
				int rsize=gm.pktsz+gm.pkto;
				if(rsize<=0){
					throw std::runtime_error("invalid generic packet record size");
				}
				int soffst=(idx-1)*rsize+1;
				b=gm.pktbas+soffst+gm.pkto;
			}
			const double*ptr=word_ptr(b,gm.pktsz);
			packet.assign(ptr,ptr+gm.pktsz);
			return;
		}

		if(gm.pdrtyp==1){
			int begin1=d_to_iword(word_at(gm.pdrbas+idx),"GENERIC PDR BEGIN");
			int begin2=d_to_iword(word_at(gm.pdrbas+idx+1),"GENERIC PDR END");
			int size=begin2-begin1-gm.pkto;
			if(size<1){
				throw std::runtime_error("invalid generic variable packet size");
			}
			int b=gm.pktbas+begin1;
			const double*ptr=word_ptr(b,size);
			packet.assign(ptr,ptr+size);
			return;
		}

		throw std::runtime_error("unsupported generic packet directory type");
	}

	void parse_segments(const std::vector<std::uint8_t>&raw){
		if(nd!=2||ni!=6){
			throw std::runtime_error(
				"unsupported DAF summary format (expect ND=2, NI=6)");
		}
		const int ss=nd+(ni+1)/2;
		if(ss<=0){
			throw std::runtime_error("invalid summary size");
		}
		const int total_words=static_cast<int>(raw.size()/8U);

		int rec=first_sum_rec;
		std::size_t chain_guard=0;
		const std::size_t max_chain=raw.size()/1024+1;
		while(rec!=0){
			++chain_guard;
			if(chain_guard>max_chain){
				throw std::runtime_error("invalid summary record chain");
			}

			if(rec<1){
				throw std::runtime_error("invalid summary record index");
			}
			std::size_t rec_off=(static_cast<std::size_t>(rec)-1U)*1024U;
			if(rec_off+1024U>raw.size()){
				throw std::runtime_error("summary record out of file range");
			}

			const std::uint8_t*rptr=raw.data()+rec_off;
			int next=d_to_iword(rd_f64(rptr+0,little),"NEXT");
			int prev=d_to_iword(rd_f64(rptr+8,little),"PREV");
			int nsum=d_to_iword(rd_f64(rptr+16,little),"NSUM");
			(void)prev;

			const int nmax=125/ss;
			if(nsum<0||nsum>nmax){
				throw std::runtime_error("invalid NSUM in summary record");
			}

			for(int i=0;i<nsum;++i){
				std::size_t sum_word=3U+static_cast<std::size_t>(i*ss);
				std::size_t sum_off=rec_off+sum_word*8U;
				if(sum_off+static_cast<std::size_t>(ss)*8U>rec_off+1024U){
					throw std::runtime_error("summary out of record bounds");
				}

				SpkSeg seg;
				seg.et_begin=rd_f64(raw.data()+sum_off+0,little);
				seg.et_end=rd_f64(raw.data()+sum_off+8,little);

				std::size_t int_off=sum_off+static_cast<std::size_t>(nd)*8U;
				seg.target=rd_i32(raw.data()+int_off+0,little);
				seg.center=rd_i32(raw.data()+int_off+4,little);
				seg.frame=rd_i32(raw.data()+int_off+8,little);
				seg.type=rd_i32(raw.data()+int_off+12,little);
				seg.begin_addr=rd_i32(raw.data()+int_off+16,little);
				seg.end_addr=rd_i32(raw.data()+int_off+20,little);

				if(seg.begin_addr<1||seg.end_addr<seg.begin_addr||
				   seg.end_addr>total_words){
					throw std::runtime_error("invalid segment address span");
				}

				std::size_t idx=segs.size();
				segs.push_back(seg);
				seg_idx[seg.target].push_back(idx);
				cover_map[seg.target].push_back({seg.et_begin,seg.et_end});
			}

			rec=next;
		}

		obj_ids.reserve(cover_map.size());
		for(auto&kv : cover_map){
			merge_cover(kv.second);
			obj_ids.push_back(kv.first);
		}
		std::sort(obj_ids.begin(),obj_ids.end());
	}

	void load(const std::string&path){
		filepath=path;

		std::ifstream ifs(filepath,std::ios::binary);
		if(!ifs){
			throw std::runtime_error("failed to open ephemeris file: "+filepath);
		}
		ifs.seekg(0,std::ios::end);
		std::streamoff sz_off=ifs.tellg();
		if(sz_off<=0){
			throw std::runtime_error("ephemeris file is empty: "+filepath);
		}
		std::size_t fsize=static_cast<std::size_t>(sz_off);
		if(fsize%8U!=0){
			throw std::runtime_error("invalid DAF file size: "+filepath);
		}
		ifs.seekg(0,std::ios::beg);

		std::vector<std::uint8_t> raw(fsize);
		if(!ifs.read(reinterpret_cast<char*>(raw.data()),
					 static_cast<std::streamsize>(raw.size()))){
			throw std::runtime_error("failed to read ephemeris file: "+filepath);
		}

		if(raw.size()<1024U){
			throw std::runtime_error("invalid DAF file (too small): "+filepath);
		}

		std::string idw(reinterpret_cast<const char*>(raw.data()),8);
		if(idw.rfind("DAF/SPK",0)!=0){
			throw std::runtime_error("not a DAF/SPK file: "+filepath);
		}

		std::string locfmt(reinterpret_cast<const char*>(raw.data()+88),8);
		if(locfmt=="LTL-IEEE"){
			little=true;
		}else if(locfmt=="BIG-IEEE"){
			little=false;
		}else{
			throw std::runtime_error("unsupported DAF binary format: "+locfmt);
		}

		nd=rd_i32(raw.data()+8,little);
		ni=rd_i32(raw.data()+12,little);
		first_sum_rec=rd_i32(raw.data()+76,little);
		last_sum_rec=rd_i32(raw.data()+80,little);
		(void)last_sum_rec;

		if(first_sum_rec<1){
			throw std::runtime_error("invalid DAF first summary record");
		}

		parse_segments(raw);

		words.resize(raw.size()/8U);
		for(std::size_t i=0;i<words.size();++i){
			words[i]=rd_f64(raw.data()+i*8U,little);
		}
	}

	const SpkSeg*pick_segment(int target,double et) const{
		auto it=seg_idx.find(target);
		if(it==seg_idx.end()){
			return nullptr;
		}
		const auto&ids=it->second;
		for(auto rit=ids.rbegin();rit!=ids.rend();++rit){
			const SpkSeg&seg=segs[*rit];
			if(et>=seg.et_begin&&et<=seg.et_end){
				return &seg;
			}
		}
		return nullptr;
	}

	State6 eval_segment(const SpkSeg&seg,double et) const{
		State6 out;
		if(seg.type==1){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<72){
				throw std::runtime_error("invalid SPK type 1 segment span");
			}

			int nrec=d_to_iword(word_at(seg.end_addr),"NREC");
			if(nrec<1){
				throw std::runtime_error("invalid SPK type 1 record count");
			}
			int ndir=nrec/100;
			int dlsize=71;
			int expect_words=nrec*dlsize+nrec+ndir+1;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 1 segment");
			}

			int epoch_base=seg.end_addr-ndir-nrec;
			std::vector<double> epochs(static_cast<std::size_t>(nrec));
			for(int i=0;i<nrec;++i){
				epochs[static_cast<std::size_t>(i)]=word_at(epoch_base+i);
			}

			int recno=
				static_cast<int>(std::lower_bound(epochs.begin(),epochs.end(),et)-
								 epochs.begin())+
				1;
			if(recno<1){
				recno=1;
			}
			if(recno>nrec){
				recno=nrec;
			}

			int recadr=seg.begin_addr+(recno-1)*dlsize;
			const double*rec=word_ptr(recadr,dlsize);
			eval_mda(15,rec,et,out);
			return out;
		}

		if(seg.type==2||seg.type==3){
			if(seg.end_addr-seg.begin_addr<3){
				throw std::runtime_error("invalid SPK segment directory span");
			}

			int seg_words=seg.end_addr-seg.begin_addr+1;
			double init=word_at(seg.end_addr-3);
			double intlen=word_at(seg.end_addr-2);
			int recsiz=d_to_iword(word_at(seg.end_addr-1),"RSIZE");
			int nrec=d_to_iword(word_at(seg.end_addr),"NREC");
			if(!std::isfinite(init)||!std::isfinite(intlen)||
			   intlen<=0.0||recsiz<=0||nrec<=0){
				throw std::runtime_error("invalid SPK segment directory values");
			}
			int expect_words=nrec*recsiz+4;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK segment directory");
			}
			if(recsiz<3){
				throw std::runtime_error("invalid SPK record size");
			}

			int recno=static_cast<int>((et-init)/intlen)+1;
			if(recno<1){
				recno=1;
			}
			if(recno>nrec){
				recno=nrec;
			}

			int recadr=(recno-1)*recsiz+seg.begin_addr;
			const double*rec=word_ptr(recadr,recsiz);

			double mid=rec[0];
			double radius=rec[1];
			if(radius<=0.0){
				throw std::runtime_error("invalid SPK record radius");
			}

			if(seg.type==2){
				if((recsiz-2)%3!=0){
					throw std::runtime_error("invalid type 2 record size");
				}
				int ncof=(recsiz-2)/3;
				if(ncof<1){
					throw std::runtime_error("invalid type 2 record size");
				}
				for(int i=0;i<3;++i){
					const double*cp=rec+2+i*ncof;
					double p=0.0;
					double v=0.0;
					chb_int(cp,ncof-1,mid,radius,et,p,v);
					if(i==0){
						out.px=p;
						out.vx=v;
					}else if(i==1){
						out.py=p;
						out.vy=v;
					}else{
						out.pz=p;
						out.vz=v;
					}
				}
				return out;
			}

			if((recsiz-2)%6!=0){
				throw std::runtime_error("invalid type 3 record size");
			}
			int ncof=(recsiz-2)/6;
			if(ncof<1){
				throw std::runtime_error("invalid type 3 record size");
			}
			for(int i=0;i<6;++i){
				const double*cp=rec+2+i*ncof;
				double p=0.0;
				chb_val(cp,ncof-1,mid,radius,et,p);
				if(i==0){
					out.px=p;
				}else if(i==1){
					out.py=p;
				}else if(i==2){
					out.pz=p;
				}else if(i==3){
					out.vx=p;
				}else if(i==4){
					out.vy=p;
				}else{
					out.vz=p;
				}
			}
			return out;
		}

		if(seg.type==5){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<2){
				throw std::runtime_error("invalid SPK type 5 segment span");
			}

			int nrec=d_to_iword(word_at(seg.end_addr),"NREC");
			double gm=word_at(seg.end_addr-1);
			if(nrec<1||!std::isfinite(gm)||gm<=0.0){
				throw std::runtime_error("invalid SPK type 5 directory");
			}

			int ndir=nrec/100;
			int expect_words=nrec*7+ndir+2;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 5 segment");
			}

			int state_base=seg.begin_addr;
			int epoch_base=seg.begin_addr+6*nrec;

			std::vector<double> epochs(static_cast<std::size_t>(nrec));
			for(int i=0;i<nrec;++i){
				epochs[static_cast<std::size_t>(i)]=word_at(epoch_base+i);
			}

			int cnt=static_cast<int>(
				std::lower_bound(epochs.begin(),epochs.end(),et)-epochs.begin());

			int idx1=0;
			int idx2=0;
			if(cnt<=0){
				idx1=1;
				idx2=1;
			}else if(cnt>=nrec){
				idx1=nrec;
				idx2=nrec;
			}else{
				idx1=cnt;
				idx2=cnt+1;
			}

			std::array<double,6> pv1{};
			std::array<double,6> pv2{};
			const double*sp1=word_ptr(state_base+(idx1-1)*6,6);
			const double*sp2=word_ptr(state_base+(idx2-1)*6,6);
			for(int i=0;i<6;++i){
				pv1[static_cast<std::size_t>(i)]=sp1[i];
				pv2[static_cast<std::size_t>(i)]=sp2[i];
			}
			double t1=epochs[static_cast<std::size_t>(idx1-1)];
			double t2=epochs[static_cast<std::size_t>(idx2-1)];

			std::array<double,6> st{};
			eval_type5_record(et,pv1,t1,pv2,t2,gm,st);
			out.px=st[0];
			out.py=st[1];
			out.pz=st[2];
			out.vx=st[3];
			out.vy=st[4];
			out.vz=st[5];
			return out;
		}

		if(seg.type==8||seg.type==12){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<4){
				throw std::runtime_error("invalid SPK type 8/12 segment span");
			}

			double start=word_at(seg.end_addr-3);
			double step=word_at(seg.end_addr-2);
			int degree=d_to_iword(word_at(seg.end_addr-1),"DEGREE");
			int n=d_to_iword(word_at(seg.end_addr),"N");
			if(!std::isfinite(start)||!std::isfinite(step)||step==0.0||
			   degree<1||n<1){
				throw std::runtime_error("invalid SPK type 8/12 directory");
			}
			int expect_words=6*n+4;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 8/12 segment");
			}

			int wndsiz=degree+1;
			if(wndsiz<1||wndsiz>n){
				throw std::runtime_error("invalid SPK type 8/12 window size");
			}

			int first=1;
			if((wndsiz&1)!=0){
				int near_i=static_cast<int>(std::round((et-start)/step))+1;
				first=std::min(std::max(1,near_i-degree/2),n-degree);
			}else{
				int low=static_cast<int>((et-start)/step)+1;
				first=std::min(std::max(1,low-degree/2),n-degree);
			}
			int last=first+degree;
			if(first<1||last>n){
				throw std::runtime_error("invalid SPK type 8/12 window");
			}

			const double*states=
				word_ptr(seg.begin_addr+(first-1)*6,wndsiz*6);
			double first_epoch=start+static_cast<double>(first-1)*step;

			if(seg.type==8){
				std::vector<double> yvals(static_cast<std::size_t>(wndsiz));
				for(int c=0;c<6;++c){
					for(int i=0;i<wndsiz;++i){
						yvals[static_cast<std::size_t>(i)]=
							states[static_cast<std::size_t>(i*6+c)];
					}
					double v=
						lgresp_eval(wndsiz,first_epoch,step,yvals.data(),et);
					if(c==0){
						out.px=v;
					}else if(c==1){
						out.py=v;
					}else if(c==2){
						out.pz=v;
					}else if(c==3){
						out.vx=v;
					}else if(c==4){
						out.vy=v;
					}else{
						out.vz=v;
					}
				}
				return out;
			}

			std::vector<double> yvals(static_cast<std::size_t>(wndsiz*2));
			for(int c=0;c<3;++c){
				for(int i=0;i<wndsiz;++i){
					yvals[static_cast<std::size_t>(i*2)]=
						states[static_cast<std::size_t>(i*6+c)];
					yvals[static_cast<std::size_t>(i*2+1)]=
						states[static_cast<std::size_t>(i*6+c+3)];
				}
				double p=0.0;
				double v=0.0;
				hrmesp_eval(wndsiz,first_epoch,step,yvals.data(),et,p,v);
				if(c==0){
					out.px=p;
					out.vx=v;
				}else if(c==1){
					out.py=p;
					out.vy=v;
				}else{
					out.pz=p;
					out.vz=v;
				}
			}
			return out;
		}

		if(seg.type==9||seg.type==13){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<2){
				throw std::runtime_error("invalid SPK type 9/13 segment span");
			}

			int degree=d_to_iword(word_at(seg.end_addr-1),"DEGREE");
			int n=d_to_iword(word_at(seg.end_addr),"N");
			if(degree<1||n<1){
				throw std::runtime_error("invalid SPK type 9/13 directory");
			}
			int wndsiz=degree+1;
			if(wndsiz<1||wndsiz>n){
				throw std::runtime_error("invalid SPK type 9/13 window size");
			}

			int ndir=(n-1)/100;
			int expect_words=6*n+n+ndir+2;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 9/13 segment");
			}

			int state_base=seg.begin_addr;
			int epoch_base=seg.begin_addr+6*n;
			int dir_base=epoch_base+n;

			int group=1;
			if(ndir>0){
				group=ndir+1;
				for(int g=1;g<=ndir;++g){
					double dir_et=word_at(dir_base+g-1);
					if(et<=dir_et){
						group=g;
						break;
					}
				}
			}

			int begidx=1;
			int endidx=std::min(n,100);
			if(group!=1){
				begidx=(group-1)*100;
				endidx=std::min(begidx+100,n);
			}
			int nbuf=endidx-begidx+1;
			if(nbuf<1){
				throw std::runtime_error("invalid SPK type 9/13 epoch group");
			}

			std::vector<double> ebuf(static_cast<std::size_t>(nbuf));
			for(int i=0;i<nbuf;++i){
				ebuf[static_cast<std::size_t>(i)]=
					word_at(epoch_base+begidx-1+i);
			}

			int i=static_cast<int>(
				std::lower_bound(ebuf.begin(),ebuf.end(),et)-ebuf.begin());
			int low=(i==0)?1:(begidx+i-1);
			int high=low+1;

			int first=1;
			if((wndsiz&1)!=0){
				int near_i=low;
				if(i>0&&i<nbuf){
					double dlow=std::fabs(et-ebuf[static_cast<std::size_t>(i-1)]);
					double dhigh=std::fabs(et-ebuf[static_cast<std::size_t>(i)]);
					if(!(dlow<dhigh)){
						near_i=high;
					}
				}
				first=std::min(std::max(near_i-degree/2,1),n-degree);
			}else{
				first=std::min(std::max(low-degree/2,1),n-degree);
			}
			int last=first+degree;
			if(first<1||last>n){
				throw std::runtime_error("invalid SPK type 9/13 window");
			}

			const double*states=
				word_ptr(state_base+(first-1)*6,wndsiz*6);
			std::vector<double> xvals(static_cast<std::size_t>(wndsiz));
			for(int k=0;k<wndsiz;++k){
				xvals[static_cast<std::size_t>(k)]=
					word_at(epoch_base+first-1+k);
			}

			if(seg.type==9){
				std::vector<double> yvals(static_cast<std::size_t>(wndsiz));
				for(int c=0;c<6;++c){
					for(int k=0;k<wndsiz;++k){
						yvals[static_cast<std::size_t>(k)]=
							states[static_cast<std::size_t>(k*6+c)];
					}
					double v=lgrint_eval(wndsiz,xvals.data(),yvals.data(),et);
					if(c==0){
						out.px=v;
					}else if(c==1){
						out.py=v;
					}else if(c==2){
						out.pz=v;
					}else if(c==3){
						out.vx=v;
					}else if(c==4){
						out.vy=v;
					}else{
						out.vz=v;
					}
				}
				return out;
			}

			std::vector<double> yvals(static_cast<std::size_t>(wndsiz*2));
			for(int c=0;c<3;++c){
				for(int k=0;k<wndsiz;++k){
					yvals[static_cast<std::size_t>(k*2)]=
						states[static_cast<std::size_t>(k*6+c)];
					yvals[static_cast<std::size_t>(k*2+1)]=
						states[static_cast<std::size_t>(k*6+c+3)];
				}
				double p=0.0;
				double v=0.0;
				hrmint_eval(wndsiz,xvals.data(),yvals.data(),et,p,v);
				if(c==0){
					out.px=p;
					out.vx=v;
				}else if(c==1){
					out.py=p;
					out.vy=v;
				}else{
					out.pz=p;
					out.vz=v;
				}
			}
			return out;
		}

		if(seg.type==10){
			throw std::runtime_error("unsupported SPK type 10 segment");
		}

		if(seg.type==14){
			GenericMeta gm=parse_generic_meta(seg);
			if(gm.ncon<1||gm.npkt<1||gm.nref<1){
				throw std::runtime_error("invalid SPK type 14 generic metadata");
			}

			int degree=d_to_iword(word_at(gm.conbas+1),"CHBDEG");
			if(degree<0){
				throw std::runtime_error("invalid SPK type 14 degree");
			}
			int ncoeff=degree+1;

			int pidx=0;
			double refval=0.0;
			if(!find_generic_ref(gm,et,pidx,refval)){
				(void)refval;
				throw std::runtime_error("no SPK type 14 packet for request epoch");
			}

			std::vector<double> packet;
			fetch_generic_packet(gm,pidx,packet);
			int need=2+ncoeff*6;
			if(static_cast<int>(packet.size())<need){
				throw std::runtime_error("invalid SPK type 14 packet size");
			}

			const double*rec=packet.data();
			double mid=rec[0];
			double radius=rec[1];
			if(!(radius>0.0)){
				throw std::runtime_error("invalid SPK type 14 packet radius");
			}

			for(int i=0;i<6;++i){
				const double*cp=rec+2+i*ncoeff;
				double v=0.0;
				chb_val(cp,degree,mid,radius,et,v);
				if(i==0){
					out.px=v;
				}else if(i==1){
					out.py=v;
				}else if(i==2){
					out.pz=v;
				}else if(i==3){
					out.vx=v;
				}else if(i==4){
					out.vy=v;
				}else{
					out.vz=v;
				}
			}
			return out;
		}

		if(seg.type==15){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words!=16){
				throw std::runtime_error("invalid SPK type 15 segment span");
			}
			const double*rec=word_ptr(seg.begin_addr,16);
			std::array<double,6> st{};
			eval_type15_record(et,rec,st);
			out.px=st[0];
			out.py=st[1];
			out.pz=st[2];
			out.vx=st[3];
			out.vy=st[4];
			out.vz=st[5];
			return out;
		}

		if(seg.type==17){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words!=12){
				throw std::runtime_error("invalid SPK type 17 segment span");
			}
			const double*rec=word_ptr(seg.begin_addr,12);
			std::array<double,6> st{};
			eval_type17_record(et,rec,st);
			out.px=st[0];
			out.py=st[1];
			out.pz=st[2];
			out.vx=st[3];
			out.vy=st[4];
			out.vz=st[5];
			return out;
		}

		if(seg.type==18){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<4){
				throw std::runtime_error("invalid SPK type 18 segment span");
			}

			int subtyp=d_to_iword(word_at(seg.end_addr-2),"SUBTYPE");
			int wndsiz=d_to_iword(word_at(seg.end_addr-1),"WNDSIZ");
			int npkt=d_to_iword(word_at(seg.end_addr),"NPKT");
			int packsz=0;
			int maxwnd=0;
			if(subtyp==0){
				packsz=12;
				maxwnd=8;
			}else if(subtyp==1){
				packsz=6;
				maxwnd=16;
			}else{
				throw std::runtime_error("unsupported SPK type 18 subtype");
			}
			if(npkt<1){
				throw std::runtime_error("invalid SPK type 18 packet count");
			}
			if(wndsiz<2||wndsiz>maxwnd||(wndsiz&1)!=0){
				throw std::runtime_error("invalid SPK type 18 window size");
			}

			int ndir=(npkt-1)/100;
			int expect_words=npkt*packsz+npkt+ndir+3;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 18 segment");
			}

			int timbas=seg.begin_addr+npkt*packsz-1;
			std::vector<double> epochs(static_cast<std::size_t>(npkt));
			for(int i=0;i<npkt;++i){
				epochs[static_cast<std::size_t>(i)]=word_at(timbas+1+i);
			}

			int lb=static_cast<int>(
				std::lower_bound(epochs.begin(),epochs.end(),et)-epochs.begin());
			int low=(lb==0)?1:lb;
			int high=low+1;

			int lsize=std::min(wndsiz/2,low);
			int rsize=std::min(wndsiz/2,npkt-high+1);
			int nrcpkt=lsize+rsize;
			if(nrcpkt<1){
				throw std::runtime_error("invalid SPK type 18 packet window");
			}
			int first=low-lsize+1;
			int last=first+nrcpkt-1;
			if(first<1||last>npkt){
				throw std::runtime_error("invalid SPK type 18 packet range");
			}

			const double*packets=
				word_ptr(seg.begin_addr+(first-1)*packsz,nrcpkt*packsz);
			std::vector<double> xvals(static_cast<std::size_t>(nrcpkt));
			for(int i=0;i<nrcpkt;++i){
				xvals[static_cast<std::size_t>(i)]=
					epochs[static_cast<std::size_t>(first-1+i)];
			}

			if(subtyp==1){
				std::vector<double> yvals(static_cast<std::size_t>(nrcpkt));
				for(int c=0;c<6;++c){
					for(int i=0;i<nrcpkt;++i){
						yvals[static_cast<std::size_t>(i)]=
							packets[static_cast<std::size_t>(i*6+c)];
					}
					double v=lgrint_eval(nrcpkt,xvals.data(),yvals.data(),et);
					if(c==0){
						out.px=v;
					}else if(c==1){
						out.py=v;
					}else if(c==2){
						out.pz=v;
					}else if(c==3){
						out.vx=v;
					}else if(c==4){
						out.vy=v;
					}else{
						out.vz=v;
					}
				}
				return out;
			}

			std::vector<double> yvals(static_cast<std::size_t>(nrcpkt*2));
			for(int c=0;c<3;++c){
				for(int i=0;i<nrcpkt;++i){
					yvals[static_cast<std::size_t>(i*2)]=
						packets[static_cast<std::size_t>(i*12+c)];
					yvals[static_cast<std::size_t>(i*2+1)]=
						packets[static_cast<std::size_t>(i*12+c+3)];
				}
				double p=0.0;
				double v=0.0;
				hrmint_eval(nrcpkt,xvals.data(),yvals.data(),et,p,v);
				if(c==0){
					out.px=p;
				}else if(c==1){
					out.py=p;
				}else{
					out.pz=p;
				}
			}
			for(int c=0;c<3;++c){
				for(int i=0;i<nrcpkt;++i){
					yvals[static_cast<std::size_t>(i*2)]=
						packets[static_cast<std::size_t>(i*12+c+6)];
					yvals[static_cast<std::size_t>(i*2+1)]=
						packets[static_cast<std::size_t>(i*12+c+9)];
				}
				double v=0.0;
				double a=0.0;
				hrmint_eval(nrcpkt,xvals.data(),yvals.data(),et,v,a);
				if(c==0){
					out.vx=v;
				}else if(c==1){
					out.vy=v;
				}else{
					out.vz=v;
				}
			}
			return out;
		}

		if(seg.type==19){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<4){
				throw std::runtime_error("invalid SPK type 19 segment span");
			}

			int isel=d_to_iword(word_at(seg.end_addr-1),"ISEL");
			int n=d_to_iword(word_at(seg.end_addr),"NINT");
			if(n<1){
				throw std::runtime_error("invalid SPK type 19 interval count");
			}
			bool pick_last=(isel==1);

			int ndir=n/100;
			int dirbas=seg.end_addr-2-(n+1)-ndir;
			int ivbas=dirbas-(n+1);
			int ptrbas=seg.end_addr-2-(n+1);
			if(ivbas<seg.begin_addr-1||ptrbas<seg.begin_addr-1){
				throw std::runtime_error("invalid SPK type 19 layout");
			}

			std::vector<double> bounds(static_cast<std::size_t>(n+1));
			for(int i=0;i<=n;++i){
				bounds[static_cast<std::size_t>(i)]=word_at(ivbas+1+i);
			}

			int miniix=0;
			if(pick_last){
				int cnt=static_cast<int>(
					std::upper_bound(bounds.begin(),bounds.end(),et)-bounds.begin());
				if(cnt<1){
					cnt=1;
				}
				if(cnt>n){
					cnt=n;
				}
				miniix=cnt;
			}else{
				int cnt=static_cast<int>(
					std::lower_bound(bounds.begin(),bounds.end(),et)-bounds.begin());
				if(cnt<1){
					cnt=1;
				}
				if(cnt>n){
					cnt=n;
				}
				miniix=cnt;
			}

			int start_rel=d_to_iword(word_at(ptrbas+miniix),"MINI BEGIN");
			int stop_rel=d_to_iword(word_at(ptrbas+miniix+1),"MINI END");
			int minib=seg.begin_addr+start_rel-1;
			int minie=seg.begin_addr+stop_rel-2;
			if(minib<seg.begin_addr||minie<minib||minie>seg.end_addr){
				throw std::runtime_error("invalid SPK type 19 mini-segment range");
			}

			int subtyp=d_to_iword(word_at(minie-2),"SUBTYPE");
			int wndsiz=d_to_iword(word_at(minie-1),"WNDSIZ");
			int npkt=d_to_iword(word_at(minie),"NPKT");
			if(subtyp<0||subtyp>2){
				throw std::runtime_error("unsupported SPK type 19 subtype");
			}
			int pktsiz=(subtyp==0)?12:6;
			int maxwnd=(subtyp==0)?14:28;
			if(npkt<1){
				throw std::runtime_error("invalid SPK type 19 packet count");
			}
			if(wndsiz<2||wndsiz>maxwnd||(wndsiz&1)!=0){
				throw std::runtime_error("invalid SPK type 19 window size");
			}

			int npkdir=(npkt-1)/100;
			int mini_words=minie-minib+1;
			int expect_mini=npkt*pktsiz+npkt+npkdir+3;
			if(expect_mini!=mini_words){
				throw std::runtime_error("inconsistent SPK type 19 mini-segment");
			}

			int timbas=minib-1+npkt*pktsiz;
			std::vector<double> epochs(static_cast<std::size_t>(npkt));
			for(int i=0;i<npkt;++i){
				epochs[static_cast<std::size_t>(i)]=word_at(timbas+1+i);
			}

			int lb=static_cast<int>(
				std::lower_bound(epochs.begin(),epochs.end(),et)-epochs.begin());
			int low=(lb==0)?1:lb;
			int high=low+1;

			int lsize=std::min(wndsiz/2,low);
			int rsize=std::min(wndsiz/2,npkt-high+1);
			int nrcpkt=lsize+rsize;
			if(nrcpkt<1){
				throw std::runtime_error("invalid SPK type 19 packet window");
			}
			int first=low-lsize+1;
			int last=first+nrcpkt-1;
			if(first<1||last>npkt){
				throw std::runtime_error("invalid SPK type 19 packet range");
			}

			const double*packets=word_ptr(minib+(first-1)*pktsiz,nrcpkt*pktsiz);
			std::vector<double> xvals(static_cast<std::size_t>(nrcpkt));
			for(int i=0;i<nrcpkt;++i){
				xvals[static_cast<std::size_t>(i)]=
					epochs[static_cast<std::size_t>(first-1+i)];
			}

			if(subtyp==1){
				std::vector<double> yvals(static_cast<std::size_t>(nrcpkt));
				for(int c=0;c<6;++c){
					for(int i=0;i<nrcpkt;++i){
						yvals[static_cast<std::size_t>(i)]=
							packets[static_cast<std::size_t>(i*6+c)];
					}
					double v=lgrint_eval(nrcpkt,xvals.data(),yvals.data(),et);
					if(c==0){
						out.px=v;
					}else if(c==1){
						out.py=v;
					}else if(c==2){
						out.pz=v;
					}else if(c==3){
						out.vx=v;
					}else if(c==4){
						out.vy=v;
					}else{
						out.vz=v;
					}
				}
				return out;
			}

			std::vector<double> yvals(static_cast<std::size_t>(nrcpkt*2));
			if(subtyp==0){
				for(int c=0;c<3;++c){
					for(int i=0;i<nrcpkt;++i){
						yvals[static_cast<std::size_t>(i*2)]=
							packets[static_cast<std::size_t>(i*12+c)];
						yvals[static_cast<std::size_t>(i*2+1)]=
							packets[static_cast<std::size_t>(i*12+c+3)];
					}
					double p=0.0;
					double v=0.0;
					hrmint_eval(nrcpkt,xvals.data(),yvals.data(),et,p,v);
					if(c==0){
						out.px=p;
					}else if(c==1){
						out.py=p;
					}else{
						out.pz=p;
					}
				}
				for(int c=0;c<3;++c){
					for(int i=0;i<nrcpkt;++i){
						yvals[static_cast<std::size_t>(i*2)]=
							packets[static_cast<std::size_t>(i*12+c+6)];
						yvals[static_cast<std::size_t>(i*2+1)]=
							packets[static_cast<std::size_t>(i*12+c+9)];
					}
					double v=0.0;
					double a=0.0;
					hrmint_eval(nrcpkt,xvals.data(),yvals.data(),et,v,a);
					if(c==0){
						out.vx=v;
					}else if(c==1){
						out.vy=v;
					}else{
						out.vz=v;
					}
				}
				return out;
			}

			for(int c=0;c<3;++c){
				for(int i=0;i<nrcpkt;++i){
					yvals[static_cast<std::size_t>(i*2)]=
						packets[static_cast<std::size_t>(i*6+c)];
					yvals[static_cast<std::size_t>(i*2+1)]=
						packets[static_cast<std::size_t>(i*6+c+3)];
				}
				double p=0.0;
				double v=0.0;
				hrmint_eval(nrcpkt,xvals.data(),yvals.data(),et,p,v);
				if(c==0){
					out.px=p;
					out.vx=v;
				}else if(c==1){
					out.py=p;
					out.vy=v;
				}else{
					out.pz=p;
					out.vz=v;
				}
			}
			return out;
		}

		if(seg.type==20){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<8){
				throw std::runtime_error("invalid SPK type 20 segment span");
			}

			double dscale=word_at(seg.end_addr-6);
			double tscale=word_at(seg.end_addr-5);
			double initjd=word_at(seg.end_addr-4);
			double initfr=word_at(seg.end_addr-3);
			double intlen=word_at(seg.end_addr-2);
			int recsiz=d_to_iword(word_at(seg.end_addr-1),"RSIZE");
			int nrec=d_to_iword(word_at(seg.end_addr),"NREC");
			if(!std::isfinite(dscale)||!std::isfinite(tscale)||
			   !std::isfinite(initjd)||!std::isfinite(initfr)||
			   !std::isfinite(intlen)||tscale==0.0||intlen<=0.0||recsiz<3||
			   nrec<1){
				throw std::runtime_error("invalid SPK type 20 directory");
			}

			if(recsiz%3!=0){
				throw std::runtime_error("invalid SPK type 20 record size");
			}
			int nterms=recsiz/3;
			if(nterms<2){
				throw std::runtime_error("invalid SPK type 20 record size");
			}

			int expect_words=nrec*recsiz+7;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 20 segment");
			}

			double init=(initjd-2451545.0+initfr)*SEC_DAY;
			double intrvl=intlen*SEC_DAY;
			int recno=static_cast<int>((et-init)/intrvl)+1;
			if(recno<1){
				recno=1;
			}
			if(recno>nrec){
				recno=nrec;
			}

			double recbeg=
				(initjd-2451545.0+static_cast<double>(recno-1)*intlen)*SEC_DAY;
			double radius=intrvl/2.0;
			double mid=recbeg+initfr*SEC_DAY+radius;

			int recadr=seg.begin_addr+(recno-1)*recsiz;
			const double*rec=word_ptr(recadr,recsiz);
			int ncof=nterms-1;
			std::vector<double> coeff(static_cast<std::size_t>(ncof));
			for(int c=0;c<3;++c){
				const double*sec=rec+c*nterms;
				for(int j=0;j<ncof;++j){
					coeff[static_cast<std::size_t>(j)]=
						sec[j]*(dscale/tscale);
				}
				double posmid=sec[ncof]*dscale;
				double vel=0.0;
				double intg=0.0;
				chb_igr(coeff.data(),ncof-1,mid,radius,et,vel,intg);
				double pos=posmid+intg;
				if(c==0){
					out.px=pos;
					out.vx=vel;
				}else if(c==1){
					out.py=pos;
					out.vy=vel;
				}else{
					out.pz=pos;
					out.vz=vel;
				}
			}
			return out;
		}

		if(seg.type==21){
			int seg_words=seg.end_addr-seg.begin_addr+1;
			if(seg_words<4){
				throw std::runtime_error("invalid SPK type 21 segment span");
			}

			int maxdim=d_to_iword(word_at(seg.end_addr-1),"MAXDIM");
			int nrec=d_to_iword(word_at(seg.end_addr),"NREC");
			if(maxdim<1||maxdim>25||nrec<1){
				throw std::runtime_error("invalid SPK type 21 directory");
			}
			int dlsize=4*maxdim+11;
			int ndir=nrec/100;
			int expect_words=nrec*dlsize+nrec+ndir+2;
			if(expect_words!=seg_words){
				throw std::runtime_error("inconsistent SPK type 21 segment");
			}

			int epoch_base=seg.end_addr-ndir-nrec-1;
			std::vector<double> epochs(static_cast<std::size_t>(nrec));
			for(int i=0;i<nrec;++i){
				epochs[static_cast<std::size_t>(i)]=word_at(epoch_base+i);
			}

			int recno=
				static_cast<int>(std::lower_bound(epochs.begin(),epochs.end(),et)-
								 epochs.begin())+
				1;
			if(recno<1){
				recno=1;
			}
			if(recno>nrec){
				recno=nrec;
			}

			int recadr=seg.begin_addr+(recno-1)*dlsize;
			const double*rec=word_ptr(recadr,dlsize);
			eval_mda(maxdim,rec,et,out);
			return out;
		}

		throw std::runtime_error(
			"unsupported SPK segment type: "+std::to_string(seg.type));
	}

	State6 to_ssb(int target,double et,std::unordered_map<int,State6>&memo,
				  std::unordered_set<int>&active) const{
		if(target==0){
			return State6{};
		}

		auto it=memo.find(target);
		if(it!=memo.end()){
			return it->second;
		}

		if(active.find(target)!=active.end()){
			throw std::runtime_error("SPK center chain has a cycle");
		}

		const SpkSeg*seg=pick_segment(target,et);
		if(seg==nullptr){
			throw std::runtime_error(
				"no SPK segment found for target "+std::to_string(target));
		}
		if(seg->frame!=1){
			throw std::runtime_error(
				"unsupported non-J2000 SPK frame id "+std::to_string(seg->frame));
		}

		active.insert(target);
		State6 rel=eval_segment(*seg,et);
		State6 ctr=to_ssb(seg->center,et,memo,active);
		active.erase(target);

		State6 abs=rel+ctr;
		memo[target]=abs;
		return abs;
	}

	State6 rel_state(int target,int observer,double et) const{
		if(target==observer){
			return State6{};
		}

		thread_local std::unordered_map<int,State6> memo;
		thread_local std::unordered_set<int> active;
		memo.clear();
		active.clear();
		if(memo.bucket_count()<32U){
			memo.reserve(32U);
		}
		if(active.bucket_count()<32U){
			active.reserve(32U);
		}
		State6 t=to_ssb(target,et,memo,active);
		State6 o=to_ssb(observer,et,memo,active);
		return t-o;
	}
};

namespace{

std::shared_ptr<SpkFile>acq_kernel(const std::string&filepath){
	std::string key=normalize_path(filepath);
	{
		std::lock_guard<std::mutex> lk(cache_mtx());
		auto it=cache_map().find(key);
		if(it!=cache_map().end()){
			std::shared_ptr<SpkFile> cached=it->second.lock();
			if(cached){
				return cached;
			}
		}
	}

	std::shared_ptr<SpkFile> fresh=std::make_shared<SpkFile>(key);
	{
		std::lock_guard<std::mutex> lk(cache_mtx());
		auto&slot=cache_map()[key];
		std::shared_ptr<SpkFile> cached=slot.lock();
		if(cached){
			return cached;
		}
		slot=fresh;
	}
	return fresh;
}

#if LUNAR_ENABLE_SERIES_FALLBACK

Mat3 transpose_mat(const Mat3&m){
	Mat3 out;
	for(int i=0;i<3;++i){
		for(int j=0;j<3;++j){
			out.m[i][j]=m.m[j][i];
		}
	}
	return out;
}

const Mat3&series_ecl_rot(){
	static const Mat3 kRot=[](){
		// The series fallback returns J2000 ecliptic rectangular coordinates,
		// while the rest of the ephemeris pipeline expects near-ICRF J2000
		// equatorial vectors before precession/nutation is applied.
		const double eps0=PrecNut::mean_obl(2451545.0);
		return transpose_mat(CoordTf::bias_mat())*CoordTf::R1(-eps0);
	}();
	return kRot;
}

StateAu rotate_series_state(const StateAu&state,const Mat3&rot){
	Vec3 pos=rot*Vec3(state.px,state.py,state.pz);
	Vec3 vel=rot*Vec3(state.vx,state.vy,state.vz);
	return {pos.x,pos.y,pos.z,vel.x,vel.y,vel.z};
}

bool code_to_vsop_body(int code,vsop87a::Body&body){
	switch(code){
		case 199:
			body=vsop87a::Body::Mercury;
			return true;
		case 299:
			body=vsop87a::Body::Venus;
			return true;
		case 399:
			body=vsop87a::Body::Earth;
			return true;
		case 499:
			body=vsop87a::Body::Mars;
			return true;
		case 599:
			body=vsop87a::Body::Jupiter;
			return true;
		case 699:
			body=vsop87a::Body::Saturn;
			return true;
		case 799:
			body=vsop87a::Body::Uranus;
			return true;
		case 899:
			body=vsop87a::Body::Neptune;
			return true;
		case 3:
			body=vsop87a::Body::EarthMoonBarycenter;
			return true;
		default:
			return false;
	}
}

StateAu eval_vsop_state(int code,double jd_tdb){
	vsop87a::Body body=vsop87a::Body::Earth;
	if(!code_to_vsop_body(code,body)){
		throw std::runtime_error("series ephemeris does not support code "+
								 std::to_string(code));
	}
	double xyz[3]={0.0,0.0,0.0};
	double vxyz[3]={0.0,0.0,0.0};
	vsop87a::EvaluateXYZ(body,jd_tdb,xyz,vxyz);
	return rotate_series_state(
		{xyz[0],xyz[1],xyz[2],vxyz[0],vxyz[1],vxyz[2]},
		series_ecl_rot());
}

StateAu eval_elp_moon_geo(double jd_tdb){
	elpmpp02::StateVector state;
	elpmpp02::Evaluate(elpmpp02::CorrectionSet::DE405,jd_tdb,state);
	return rotate_series_state(
		{
			state.position_km[0]/AU_KM,
			state.position_km[1]/AU_KM,
			state.position_km[2]/AU_KM,
			state.velocity_km_per_day[0]/AU_KM,
			state.velocity_km_per_day[1]/AU_KM,
			state.velocity_km_per_day[2]/AU_KM,
		},
		series_ecl_rot());
}

StateAu series_abs_state(int target,double jd_tdb){
	thread_local double memo_jd=std::numeric_limits<double>::quiet_NaN();
	thread_local std::unordered_map<int,StateAu> memo;
	if(memo_jd!=jd_tdb){
		memo.clear();
		memo_jd=jd_tdb;
	}
	auto it=memo.find(target);
	if(it!=memo.end()){
		return it->second;
	}

	StateAu out;
	switch(target){
		case 0:
		case 10:
			break;
		case 199:
		case 299:
		case 399:
		case 499:
		case 599:
		case 699:
		case 799:
		case 899:
		case 3:
			out=eval_vsop_state(target,jd_tdb);
			break;
		case 301:
			out=series_abs_state(399,jd_tdb)+eval_elp_moon_geo(jd_tdb);
			break;
		default:
			throw std::runtime_error("series ephemeris does not support code "+
									 std::to_string(target));
	}
	memo[target]=out;
	return out;
}

StateAu series_rel_state(int target,int observer,double jd_tdb){
	if(target==observer){
		return StateAu{};
	}
	StateAu t=series_abs_state(target,jd_tdb);
	StateAu o=series_abs_state(observer,jd_tdb);
	return t-o;
}

bool should_use_series_backend(const std::string&filepath){
	if(is_series_ephem(filepath)||filepath.empty()){
		return true;
	}
	return false;
}

const std::vector<int>&series_object_ids(){
	static const std::vector<int> ids={10,3,301,399,199,299,499,599,699,799,899};
	return ids;
}

#endif

}

EphRead::EphRead(const std::string&path){
	filepath=is_series_ephem(path)?kSeriesEphemToken:path;
#if !LUNAR_ENABLE_SERIES_FALLBACK
	if(filepath.empty()){
		throw std::runtime_error("ephemeris path is empty");
	}
#endif
	SSB=0;
	SUN=10;
	EMB=3;
	EARTH=399;
	MOON=301;

	id_name[SSB]="SOLAR SYSTEM BARYCENTER";
	id_name[SUN]="SUN";
	id_name[199]="MERCURY";
	id_name[299]="VENUS";
	id_name[EMB]="EARTH BARYCENTER";
	id_name[EARTH]="EARTH";
	id_name[MOON]="MOON";
	id_name[499]="MARS";
	id_name[599]="JUPITER";
	id_name[699]="SATURN";
	id_name[799]="URANUS";
	id_name[899]="NEPTUNE";

	load_kern();
}

void EphRead::load_kern(){
	std::call_once(kern_flag,[this](){
#if LUNAR_ENABLE_SERIES_FALLBACK
		if(should_use_series_backend(filepath)){
			use_series=true;
			if(filepath.empty()){
				filepath=kSeriesEphemToken;
			}
			return;
		}
#endif
		if(!fs::exists(filepath)){
			throw std::runtime_error("ephemeris file not found: "+filepath);
		}

		std::error_code ec;
		auto fsize=fs::file_size(filepath,ec);
		if(ec||fsize==0){
			throw std::runtime_error("ephemeris file is not readable or empty: "+
									 filepath);
		}

		kern=acq_kernel(filepath);
	});

	if(use_series){
		return;
	}
	if(!kern){
		throw std::runtime_error("failed to initialize ephemeris kernel");
	}
}

std::string EphRead::to_name(int code) const{
	auto it=id_name.find(code);
	if(it==id_name.end()){
		throw std::runtime_error("Unknown target/observer code");
	}
	return it->second;
}

void EphRead::val_kern(){
	load_kern();
	if(use_series){
		return;
	}
	if(kern->obj_ids.empty()){
		throw std::runtime_error("No SPK segments found in ephemeris "+filepath);
	}
}

double EphRead::et_fromjd(double jd_tdb){ return (jd_tdb-2451545.0)*SEC_DAY; }

std::pair<PosKm3,VelKmSec3> EphRead::get_state_et_km(int target,int observer,
													  double et_tdb){
	load_kern();
#if LUNAR_ENABLE_SERIES_FALLBACK
	if(use_series){
		double jd_tdb=2451545.0+et_tdb/SEC_DAY;
		StateAu st=series_rel_state(target,observer,jd_tdb);
		PosKm3 pos(st.px*AU_KM,st.py*AU_KM,st.pz*AU_KM);
		VelKmSec3 vel(st.vx*(AU_KM/SEC_DAY),st.vy*(AU_KM/SEC_DAY),
					  st.vz*(AU_KM/SEC_DAY));
		return {pos,vel};
	}
#endif
	State6 st=kern->rel_state(target,observer,et_tdb);
	PosKm3 pos(st.px,st.py,st.pz);
	VelKmSec3 vel(st.vx,st.vy,st.vz);
	return {pos,vel};
}

PosKm3 EphRead::get_pos_et_km(int target,int observer,double et_tdb){
	return get_state_et_km(target,observer,et_tdb).first;
}

VelKmSec3 EphRead::get_vel_et_kms(int target,int observer,double et_tdb){
	return get_state_et_km(target,observer,et_tdb).second;
}

std::pair<Pos3,Vel3> EphRead::get_state(int target,int observer,double jd_tdb){
#if LUNAR_ENABLE_SERIES_FALLBACK
	load_kern();
	if(use_series){
		StateAu st=series_rel_state(target,observer,jd_tdb);
		Pos3 pos(st.px,st.py,st.pz);
		Vel3 vel(st.vx,st.vy,st.vz);
		return {pos,vel};
	}
#endif
	double et=et_fromjd(jd_tdb);
	auto st=get_state_et_km(target,observer,et);
	Pos3 pos(st.first.x/AU_KM,st.first.y/AU_KM,st.first.z/AU_KM);
	Vel3 vel(st.second.x*(SEC_DAY/AU_KM),st.second.y*(SEC_DAY/AU_KM),
			 st.second.z*(SEC_DAY/AU_KM));
	return {pos,vel};
}

Pos3 EphRead::get_pos(int target,int observer,double jd_tdb){
	return get_state(target,observer,jd_tdb).first;
}

Vel3 EphRead::get_vel(int target,int observer,double jd_tdb){
	return get_state(target,observer,jd_tdb).second;
}

std::vector<int> EphRead::spk_objects(){
	load_kern();
#if LUNAR_ENABLE_SERIES_FALLBACK
	if(use_series){
		return series_object_ids();
	}
#endif
	return kern->obj_ids;
}

std::vector<std::pair<double,double>> EphRead::spk_coverage(int obj){
	load_kern();
#if LUNAR_ENABLE_SERIES_FALLBACK
	if(use_series){
		(void)obj;
		return {};
	}
#endif
	auto it=kern->cover_map.find(obj);
	if(it==kern->cover_map.end()){
		return {};
	}
	return it->second;
}
