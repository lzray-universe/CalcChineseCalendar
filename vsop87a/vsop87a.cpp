#include "vsop87a.hpp"

#include<cmath>
#include<stdexcept>

namespace vsop87a{

extern const BodyData kMercuryData;
extern const BodyData kVenusData;
extern const BodyData kEarthData;
extern const BodyData kMarsData;
extern const BodyData kJupiterData;
extern const BodyData kSaturnData;
extern const BodyData kUranusData;
extern const BodyData kNeptuneData;
extern const BodyData kEarthMoonBarycenterData;

namespace{

const BodyData*LookupBody(Body body){
	switch(body){
	case Body::Mercury:
		return &kMercuryData;
	case Body::Venus:
		return &kVenusData;
	case Body::Earth:
		return &kEarthData;
	case Body::Mars:
		return &kMarsData;
	case Body::Jupiter:
		return &kJupiterData;
	case Body::Saturn:
		return &kSaturnData;
	case Body::Uranus:
		return &kUranusData;
	case Body::Neptune:
		return &kNeptuneData;
	case Body::EarthMoonBarycenter:
		return &kEarthMoonBarycenterData;
	}
	return nullptr;
}

void EvaluateCoordinate(const Series (&series)[kOrderCount],double T,
						double*value,double*derivative_wrt_T){
	double tp[kOrderCount]={1.0,T,0.0,0.0,0.0,0.0};
	for(int i=2;i<kOrderCount;++i){
		tp[i]=tp[i-1]*T;
	}

	double v=0.0;
	double dv=0.0;
	for(int alpha=0;alpha<kOrderCount;++alpha){
		const Series&s=series[alpha];
		for(std::uint32_t i=0;i<s.count;++i){
			const Term&term=s.terms[i];
			const double u=term.B+term.C*T;
			const double cu=std::cos(u);
			const double su=std::sin(u);
			const double base=term.A*cu;
			v+=tp[alpha]*base;
			if(alpha>0){
				dv+=alpha*tp[alpha-1]*base;
			}
			dv-=tp[alpha]*term.A*term.C*su;
		}
	}
	*value=v;
	*derivative_wrt_T=dv;
}
}

const BodyData&GetBodyData(Body body){
	const BodyData*ptr=LookupBody(body);
	if(!ptr){
		throw std::runtime_error("Unknown VSOP87A body");
	}
	return *ptr;
}

void EvaluateXYZ(Body body,double jd_tdb,double out_xyz_au[3],
				 double out_vxyz_au_per_day[3]){
	const BodyData&data=GetBodyData(body);
	const double T=(jd_tdb-kJ2000)/kDaysPerMillennium;

	for(int coord=0;coord<kCoordinateCount;++coord){
		double value=0.0;
		double derivative=0.0;
		EvaluateCoordinate(data.coord[coord],T,&value,&derivative);
		out_xyz_au[coord]=value;
		out_vxyz_au_per_day[coord]=derivative/kDaysPerMillennium;
	}
}

}
