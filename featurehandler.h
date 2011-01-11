
#include <ogrsf_frmts.h>


class FeatureHandler {
public:
    FeatureHandler(OGRDataSource *);
    ~FeatureHandler();
private:
    OGRDataSource *_dataSource;

};
