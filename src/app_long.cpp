#include "lunar/app_long.hpp"

#include<algorithm>
#include<cmath>

double AberCorr::lightday(const Vec3&vec){
	double r=vec.norm();
	return r/C_AUDAY;
}

RetProp AberCorr::geo_prop(EphRead&eph,int target,double jd_tdb,int max_iter){
	double tr=jd_tdb;

	for(int i=0;i<max_iter;++i){
		Vec3 xt=eph.get_pos(target,eph.SSB,tr);
		Vec3 xE=eph.get_pos(eph.EARTH,eph.SSB,tr);
		Vec3 X=xt-xE;
		double lt=lightday(X);
		double tr_new=jd_tdb-lt;
		if(std::fabs(tr_new-tr)<1e-12){
			tr=tr_new;
			break;
		}
		tr=tr_new;
	}

	Vec3 xt=eph.get_pos(target,eph.SSB,tr);
	Vec3 xE=eph.get_pos(eph.EARTH,eph.SSB,tr);
	Vec3 X=xt-xE;

	Vec3 vt=eph.get_vel(target,eph.SSB,tr);
	Vec3 vE=eph.get_vel(eph.EARTH,eph.SSB,tr);
	Vec3 V=vt-vE;

	return {X,V,tr};
}

Vec3 AberCorr::geo_app(EphRead&eph,int target,double jd_tdb,int max_iter){
	return geo_app(eph,target,jd_tdb,nullptr,max_iter);
}

Vec3 AberCorr::geo_app(EphRead&eph,int target,double jd_tdb,double*tr_out,
					   int max_iter){
	Vec3 xE_t=eph.get_pos(eph.EARTH,eph.SSB,jd_tdb);
	Vec3 vE_t=eph.get_vel(eph.EARTH,eph.SSB,jd_tdb);

	double tr=jd_tdb;
	Vec3 xt=eph.get_pos(target,eph.SSB,tr);
	for(int i=0;i<max_iter;++i){
		Vec3 r_geo=xt-xE_t;
		double lt=lightday(r_geo);
		double tr_new=jd_tdb-lt;
		if(std::fabs(tr_new-tr)<1e-12){
			tr=tr_new;
			break;
		}
		tr=tr_new;
		xt=eph.get_pos(target,eph.SSB,tr);
	}

	xt=eph.get_pos(target,eph.SSB,tr);

	Vec3 r_geo=xt-xE_t;
	double r=r_geo.norm();
	Vec3 n=r_geo/r;

	Vec3 beta=vE_t/C_AUDAY;
	double beta2=Vec3::dot(beta,beta);
	double gamma_inv=std::sqrt(std::max(0.0,1.0-beta2));
	double nb=Vec3::dot(n,beta);

	Vec3 n_app=(gamma_inv*n+beta+(nb*beta)/(1.0+gamma_inv))/(1.0+nb);

	double n_app_norm=n_app.norm();
	if(n_app_norm==0.0){
		if(tr_out){
			*tr_out=tr;
		}
		return n*r;
	}
	if(tr_out){
		*tr_out=tr;
	}
	return (n_app/n_app_norm)*r;
}

AppLon::AppLon(EphRead&reader)
	: eph(reader),frame_bias(CoordTf::bias_mat()),prec_ok(false),r1n_ok(false),
	  rot_ok(false){}

double AppLon::epsA(double jd_tdb){ return PrecNut::mean_obl(jd_tdb); }

Mat3 AppLon::R1_eps_N(double jd_tdb){
	if(!r1n_ok||r1n_jd!=jd_tdb){
		double epsA_val=PrecNut::mean_obl(jd_tdb);
		auto nd=PrecNut::nut_ang(jd_tdb);
		double deps=nd.second;
		double eps=epsA_val+deps;
		Mat3 R1e=CoordTf::R1(eps);
		Mat3 N=PrecNut::nut_mat(jd_tdb);
		r1n_cache=R1e*N;
		r1n_jd=jd_tdb;
		r1n_ok=true;
	}
	return r1n_cache;
}

Mat3 AppLon::prec_mat(double jd_tdb){
	if(!prec_ok||prec_jd!=jd_tdb){
		prec_cache=PrecNut::prec_mat(jd_tdb);
		prec_jd=jd_tdb;
		prec_ok=true;
	}
	return prec_cache;
}

Mat3 AppLon::rot_mat(double jd_tdb){
	if(!rot_ok||rot_jd!=jd_tdb){
		Mat3 P=prec_mat(jd_tdb);
		Mat3 R1N=R1_eps_N(jd_tdb);
		rot_cache=R1N*P*frame_bias;
		rot_jd=jd_tdb;
		rot_ok=true;
	}
	return rot_cache;
}

std::pair<double,double> AppLon::sun_calc(double jd_tdb){
	RetProp st=AberCorr::geo_prop(eph,eph.SUN,jd_tdb);

	Mat3 R=rot_mat(jd_tdb);
	Vec3 Xec=R*st.X;
	double lam=std::atan2(Xec.y,Xec.x);
	if(lam<0){
		lam+=TWO_PI;
	}

	Vec3 Xec_dot=R*st.V;

	double denom=Xec.x*Xec.x+Xec.y*Xec.y;
	double lam_dot=0.0;
	if(denom!=0.0){
		lam_dot=(Xec.x*Xec_dot.y-Xec.y*Xec_dot.x)/denom;
	}
	return {lam,lam_dot};
}

std::pair<double,double> AppLon::moon_calc(double jd_tdb){
	RetProp st=AberCorr::geo_prop(eph,eph.MOON,jd_tdb);

	Mat3 R=rot_mat(jd_tdb);
	Vec3 Xec=R*st.X;
	double lam=std::atan2(Xec.y,Xec.x);
	if(lam<0){
		lam+=TWO_PI;
	}

	Vec3 Xec_dot=R*st.V;

	double denom=Xec.x*Xec.x+Xec.y*Xec.y;
	double lam_dot=0.0;
	if(denom!=0.0){
		lam_dot=(Xec.x*Xec_dot.y-Xec.y*Xec_dot.x)/denom;
	}
	return {lam,lam_dot};
}
