#ifndef _GEOHANDLER_H_
#define _GEOHANDLER_H_

#include <vector>
#include <ogrsf_frmts.h>
#include "geopoint.h"
#include "geosegment.h"

namespace std { using namespace __gnu_cxx; }

class GeoHandler {
public:
    GeoHandler(OGRDataSource *);
    ~GeoHandler();
    int MakeConnectedNode(OGRPoint *p);
    int HandleGeometry(OGRFeature *poFeature);
    int ReadGeometry();
    GeoRef* ProcessFeature(OGRFeature *);
    int GetGID();
    int LookupPoint(const GeoPoint *p);
    GeoPoint* CreatePoint(OGRPoint *, PointType, GeoRef *);
    int AddLineString(const OGRLineString *ls, GeoSegment *, GeoRef *);
    vector<GeoRef *> GetGeoReferences(OGRGeometry *geometry);
    
private:
    OGRDataSource *_dataSource;
    int _gID;
    //GeoPoint ** _pointList;
    int _pointListSize;
    int _pointListAllocSize;
    vector<GeoPoint *> _pointList;
    vector<GeoRef *> _refList;
    double _tolerance;
};


#endif
