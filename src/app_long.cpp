#include "lunar/app_long.hpp"
#include "lunar/precnut_core.hpp"
#include "lunar/time_scale.hpp"

#include<algorithm>
#include<cmath>

namespace{

double norm2pi(double angle){
	double v=std::fmod(angle,TWO_PI);
	if(v<0.0){
		v+=TWO_PI;
	}
	return v;
}

double norm_pm_pi(double angle){
	double v=norm2pi(angle+PI);
	return v-PI;
}

double day_frac_utc(double jd_utc){
	double frac=std::fmod(jd_utc+0.5,1.0);
	if(frac<0.0){
		frac+=1.0;
	}
	return frac;
}

Vec3 cross(const Vec3&a,const Vec3&b){
	return Vec3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
}

struct DirectionState{
	Vec3 u;
	Vec3 u_dot;
};

struct LtWork{
	AberState state;
	Pos3 target_pos;
	Vel3 target_vel;
	Pos3 observer_pos;
	Vel3 observer_vel;
};

LtWork solve_geo_lt(EphRead&eph,int target,double jd_tdb,int max_iter){
	auto observer=eph.get_state(eph.EARTH,eph.SSB,jd_tdb);
	double tr=jd_tdb;
	for(int i=0;i<std::max(1,max_iter);++i){
		Pos3 target_pos=eph.get_pos(target,eph.SSB,tr);
		Pos3 sight=target_pos-observer.first;
		double tr_new=jd_tdb-AberCorr::lightday(sight);
		double change=std::fabs(tr_new-tr);
		tr=tr_new;
		if(change<1e-12){
			break;
		}
	}

	// The loop above deliberately keeps the observer at the reception epoch.
	// Finish with the state at the converged emission epoch and the exact
	// derivative of the Newtonian light-time equation.
	auto emitted=eph.get_state(target,eph.SSB,tr);
	Pos3 sight=emitted.first-observer.first;
	double range=sight.norm();
	Vec3 n=range>0.0?raw_vec(sight)/range:Vec3(1.0,0.0,0.0);
	double target_radial=Vec3::dot(n,raw_vec(emitted.second));
	double relative_radial=
		Vec3::dot(n,raw_vec(emitted.second-observer.second));
	double lt_rate=relative_radial/(C_AUDAY+target_radial);
	double tr_rate=1.0-lt_rate;
	Vel3 sight_rate=emitted.second*tr_rate-observer.second;

	LtWork out;
	out.state={sight,sight_rate,tr,tr_rate};
	out.target_pos=emitted.first;
	out.target_vel=emitted.second;
	out.observer_pos=observer.first;
	out.observer_vel=observer.second;
	return out;
}

DirectionState normalized_state(const Vec3&v,const Vec3&v_dot){
	double n=v.norm();
	if(!(n>0.0)){
		return {Vec3(1.0,0.0,0.0),Vec3()};
	}
	Vec3 u=v/n;
	Vec3 u_dot=(v_dot-u*Vec3::dot(u,v_dot))/n;
	return {u,u_dot};
}

DirectionState solar_deflection(EphRead&eph,int target,double jd_tdb,
								const LtWork&lt){
	DirectionState p=normalized_state(raw_vec(lt.state.X),raw_vec(lt.state.V));
	if(target==eph.SUN){
		return p;
	}

	auto sun=eph.get_state(eph.SUN,eph.SSB,jd_tdb);
	Vec3 source=raw_vec(lt.target_pos-sun.first);
	Vec3 source_dot=
		raw_vec(lt.target_vel)*lt.state.tr_rate-raw_vec(sun.second);
	DirectionState q=normalized_state(source,source_dot);

	Vec3 deflector_to_observer=raw_vec(lt.observer_pos-sun.first);
	Vec3 deflector_to_observer_dot=raw_vec(lt.observer_vel-sun.second);
	double em=deflector_to_observer.norm();
	if(!(em>0.0)||!(source.norm()>0.0)){
		return p;
	}
	DirectionState e=
		normalized_state(deflector_to_observer,deflector_to_observer_dot);
	double em_dot=Vec3::dot(e.u,deflector_to_observer_dot);

	Vec3 qpe=q.u+e.u;
	double geometry=Vec3::dot(q.u,qpe);
	double geometry_dot=Vec3::dot(q.u_dot,qpe)+
		Vec3::dot(q.u,q.u_dot+e.u_dot);
	double em2=em*em;
	double limiter=(em2>1.0)?1e-6/em2:1e-6;
	double limiter_dot=(em2>1.0)?-2.0*limiter*em_dot/em:0.0;
	double denom=geometry>limiter?geometry:limiter;
	double denom_dot=geometry>limiter?geometry_dot:limiter_dot;

	constexpr double solar_schwarzschild_au=1.97412574336e-8;
	double scale=solar_schwarzschild_au/(em*denom);
	double scale_dot=scale*(-em_dot/em-denom_dot/denom);
	Vec3 exq=cross(e.u,q.u);
	Vec3 exq_dot=cross(e.u_dot,q.u)+cross(e.u,q.u_dot);
	Vec3 correction=cross(p.u,exq);
	Vec3 correction_dot=cross(p.u_dot,exq)+cross(p.u,exq_dot);
	return normalized_state(p.u+correction*scale,
		p.u_dot+correction*scale_dot+correction_dot*scale);
}

DirectionState aberration(const DirectionState&natural,const Vec3&beta,
						  const Vec3&beta_dot){
	double beta2=Vec3::dot(beta,beta);
	if(!(beta2<1.0)){
		return natural;
	}
	double gamma_inv=std::sqrt(std::max(0.0,1.0-beta2));
	if(!(gamma_inv>0.0)){
		return natural;
	}
	double gamma_inv_dot=-Vec3::dot(beta,beta_dot)/gamma_inv;
	double ub=Vec3::dot(natural.u,beta);
	double ub_dot=Vec3::dot(natural.u_dot,beta)+
		Vec3::dot(natural.u,beta_dot);
	double one_plus_gamma=1.0+gamma_inv;
	double k=ub/one_plus_gamma;
	double k_dot=(ub_dot*one_plus_gamma-ub*gamma_inv_dot)/
		(one_plus_gamma*one_plus_gamma);
	double den=1.0+ub;
	if(std::fabs(den)<1e-15){
		return natural;
	}
	Vec3 numerator=natural.u*gamma_inv+beta+beta*k;
	Vec3 numerator_dot=natural.u_dot*gamma_inv+
		natural.u*gamma_inv_dot+beta_dot+beta_dot*k+beta*k_dot;
	Vec3 app=numerator/den;
	Vec3 app_dot=(numerator_dot*den-numerator*ub_dot)/(den*den);
	return normalized_state(app,app_dot);
}

Vec3 observer_acceleration(EphRead&eph,double jd_tdb){
	constexpr double h=1e-3;
	Vel3 vm=eph.get_state(eph.EARTH,eph.SSB,jd_tdb-h).second;
	Vel3 vp=eph.get_state(eph.EARTH,eph.SSB,jd_tdb+h).second;
	return raw_vec(vp-vm)/(2.0*h);
}

}

double AberCorr::lightday(const Pos3&vec){
	double r=vec.norm();
	return r/C_AUDAY;
}

AberState AberCorr::geo_geom_state(EphRead&eph,int target,double jd_tdb){
	auto target_state=eph.get_state(target,eph.SSB,jd_tdb);
	auto earth_state=eph.get_state(eph.EARTH,eph.SSB,jd_tdb);
	return {target_state.first-earth_state.first,
			target_state.second-earth_state.second,jd_tdb,1.0};
}

AberState AberCorr::geo_lt_state(EphRead&eph,int target,double jd_tdb,
								 int max_iter){
	return solve_geo_lt(eph,target,jd_tdb,max_iter).state;
}

Pos3 AberCorr::geo_app(EphRead&eph,int target,double jd_tdb,int max_iter){
	return geo_app(eph,target,jd_tdb,nullptr,max_iter);
}

Pos3 AberCorr::geo_app(EphRead&eph,int target,double jd_tdb,double*tr_out,
					   int max_iter){
	LtWork lt=solve_geo_lt(eph,target,jd_tdb,max_iter);
	DirectionState natural=solar_deflection(eph,target,jd_tdb,lt);
	DirectionState app=aberration(
		natural,raw_vec(lt.observer_vel)/C_AUDAY,Vec3());
	double range=lt.state.X.norm();
	if(tr_out){
		*tr_out=lt.state.tr;
	}
	return pos3(app.u*range);
}

AberState AberCorr::geo_app_state(EphRead&eph,int target,double jd_tdb,
								  int max_iter){
	LtWork lt=solve_geo_lt(eph,target,jd_tdb,max_iter);
	DirectionState natural=solar_deflection(eph,target,jd_tdb,lt);
	Vec3 beta=raw_vec(lt.observer_vel)/C_AUDAY;
	Vec3 beta_dot=observer_acceleration(eph,jd_tdb)/C_AUDAY;
	DirectionState app=aberration(natural,beta,beta_dot);

	double range=lt.state.X.norm();
	DirectionState lt_direction=
		normalized_state(raw_vec(lt.state.X),raw_vec(lt.state.V));
	double range_rate=Vec3::dot(lt_direction.u,raw_vec(lt.state.V));
	Pos3 position=pos3(app.u*range);
	Vel3 velocity=vel3(app.u_dot*range+app.u*range_rate);
	return {position,velocity,lt.state.tr,lt.state.tr_rate};
}

AppLon::AppLon(EphRead&reader)
	: eph(reader),prec_ok(false),r1n_ok(false),rot_ok(false){}

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
		rot_cache=R1N*P;
		rot_jd=jd_tdb;
		rot_ok=true;
	}
	return rot_cache;
}

double AppLon::body_lam_app(int target,double jd_tdb){
	Pos3 apparent=AberCorr::geo_app(eph,target,jd_tdb,6);
	Vec3 ecliptic=raw_vec(rot_mat(jd_tdb)*apparent);
	return norm2pi(std::atan2(ecliptic.y,ecliptic.x));
}

std::pair<double,double> AppLon::sun_calc(double jd_tdb){
	double longitude=body_lam_app(eph.SUN,jd_tdb);
	constexpr double h=1e-4;
	double before=body_lam_app(eph.SUN,jd_tdb-h);
	double after=body_lam_app(eph.SUN,jd_tdb+h);
	return {longitude,norm_pm_pi(after-before)/(2.0*h)};
}

std::pair<double,double> AppLon::moon_calc(double jd_tdb){
	double longitude=body_lam_app(eph.MOON,jd_tdb);
	constexpr double h=1e-4;
	double before=body_lam_app(eph.MOON,jd_tdb-h);
	double after=body_lam_app(eph.MOON,jd_tdb+h);
	return {longitude,norm_pm_pi(after-before)/(2.0*h)};
}

EoTData AppLon::eot_calc(double jd_utc,double lon_deg){
	EoTData out;
	out.jd_utc=jd_utc;
	out.jd_tdb=TimeScale::utc_to_tdb(jd_utc);
	out.lon_deg=lon_deg;
	out.lon_rad=norm_pm_pi(lon_deg*PI/180.0);

	Vec3 sun_app=raw_vec(AberCorr::geo_app(eph,eph.SUN,out.jd_tdb,6));

	Mat3 P=prec_mat(out.jd_tdb);
	Mat3 N=PrecNut::nut_mat(out.jd_tdb);
	Mat3 eq_true=N*P;
	Vec3 sun_eq=eq_true*sun_app;
	double ra_app=norm2pi(std::atan2(sun_eq.y,sun_eq.x));

	double jd_ut1=TimeScale::tdb_to_ut1(out.jd_tdb);
	double uta=std::floor(jd_ut1);
	double utb=jd_ut1-uta;
	double jd_tt=TimeScale::tdb_to_tt(out.jd_tdb);
	double tta=std::floor(jd_tt);
	double ttb=jd_tt-tta;
	double gast=lunar::precnut::gst06a(uta,utb,tta,ttb);

	double h_app=gast+out.lon_rad-ra_app;
	out.apparent_solar_time_rad=norm2pi(h_app+PI);

	double lmst=day_frac_utc(jd_ut1)*TWO_PI+out.lon_rad;
	out.mean_solar_time_rad=norm2pi(lmst);

	out.eot_rad=norm_pm_pi(out.apparent_solar_time_rad-out.mean_solar_time_rad);
	out.eot_seconds=out.eot_rad*SEC_DAY/TWO_PI;
	out.eot_minutes=out.eot_seconds/60.0;
	return out;
}
