
#include <ogrsf_frmts.h>

class GeoHandler {
public:
    GeoHandler(OGRDataSource *);
    ~GeoHandler();
    int MakeConnectedNode(OGRPoint *p);
    int HandleGeometry(OGRFeature *poFeature);
    int ReadGeometry();
    int GetGID();
private:
    OGRDataSource *_dataSource;
    int _gID;


};
