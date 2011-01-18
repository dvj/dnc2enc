#include <map>
#include <vector>
#include <ogrsf_frmts.h>
#include "georef.h"

using namespace std;

class FeatureHandler {
public:
    FeatureHandler(OGRDataSource *,OGRDataSource *);
    ~FeatureHandler();
    void ReProcessFeature(OGRFeature *feature, int);
    void WriteFeatureRecords(vector<GeoRef *>, int );
    GeoRef * LookupGeoRef(int id);
private:
    OGRDataSource *_dataSource;
    OGRDataSource *_outputSource;
    int _globalSegmentID;
    map<string,int> _lookuptable; //this should probably go in a diff class   
    vector<GeoRef*> _refList;

};
