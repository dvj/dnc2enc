

#include "geopoint.h"


GeoPoint::GeoPoint() {


}

GeoPoint::GeoPoint(double x, double y, PointType pType) {
    _p[0] = x;
    _p[1] = y;
    _depth = 0;
    type = pType;
    _ogrPointRef = NULL;
}


GeoPoint::~GeoPoint() {


}


void GeoPoint::SetOGRPointReference(OGRPoint *pr) {
    _ogrPointRef = pr;



}
