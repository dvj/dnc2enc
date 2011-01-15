#ifndef _GEOHANDLER_H_
#define _GEOHANDLER_H_

#include <vector>
#include <map>
#include <ogrsf_frmts.h>
#include "geopoint.h"
#include "geosegment.h"

namespace std { using namespace __gnu_cxx; }

class GeoHandler {
public:
    GeoHandler(OGRDataSource *, OGRDataSource *);
    ~GeoHandler();
    int MakeConnectedNode(const OGRPoint *p);
    int HandleGeometry(OGRFeature *poFeature);
    int ReadGeometry();
    GeoRef* ProcessFeature(OGRFeature *);
    void ReProcessFeature(OGRFeature *);
    int GetGID();
    GeoRef * LookupGeoRef(int id);
    int LookupPoint(const GeoPoint *p);
    GeoPoint* CreatePoint(OGRPoint *, PointType, GeoRef *);
    int AddLineString(const OGRLineString *ls, GeoSegment *, GeoRef *);
    vector<GeoRef *> GetGeoReferences(OGRGeometry *geometry);
    int WriteVectorRecords(vector<GeoRef *> vrecs);
    void WriteFeatureRecords();
private:
    OGRDataSource *_dataSource;
    OGRDataSource *_outputSource;
    int _globalFeatureID;
    int _globalSegmentID;
    //GeoPoint ** _pointList;
    int _pointListSize;
    int _pointListAllocSize;
    vector<GeoPoint *> _pointList;
    vector<GeoRef *> _refList;
    double _tolerance;
    map<string,int> _lookuptable; //this should probably go in a diff class   
};


#endif
