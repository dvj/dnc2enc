#ifndef _GEOPOINT_H_
#define _GEOPOINT_H_

#include <vector>
#include "ogrsf_frmts.h"

using namespace std;

enum PointType {
    IsolatedPoint = 0,
    ConnectedPoint,
    EdgePoint
};


class GeoPoint {
public:
    GeoPoint();
    GeoPoint(double, double, PointType);
    ~GeoPoint();
    PointType type;
    double _p[2];
    void SetOGRPointReference(OGRPoint *);
private:
    double _depth;
    OGRPoint *_ogrPointRef;
};


#endif
