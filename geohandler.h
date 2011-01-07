#ifndef _GEOHANDLER_H_
#define _GEOHANDLER_H_

#include <vector>
#include <ogrsf_frmts.h>
#include "geopoint.h"
#include "geosegment.h"


class GeoHandler {
public:
    GeoHandler(OGRDataSource *);
    ~GeoHandler();
    int MakeConnectedNode(OGRPoint *p);
    int HandleGeometry(OGRFeature *poFeature);
    int ReadGeometry();
    int ProcessFeature(OGRFeature *);
    int GetGID();
    vector<GeoRef *> GetGeoReferences(OGRGeometry *geometry);
    
private:
    OGRDataSource *_dataSource;
    int _gID;
    vector<GeoPoint *> pointList;
    vector<GeoRef *> refList;
};


#endif
