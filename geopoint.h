#ifndef _GEOPOINT_H_
#define _GEOPOINT_H_

#include <vector>
#include "ogrsf_frmts.h"
#include "georef.h"

#define _Gtolerance .0000001

using namespace std;

enum PointType {
    IsolatedPoint = 0,
    ConnectedPoint,
    EdgePoint
};

class GeoPoint {
public:
    GeoPoint();
    GeoPoint(double, double, PointType, GeoRef *);
    ~GeoPoint();
    void SetOGRPointReference(OGRPoint *);
    bool operator==(const GeoPoint &) const;
    bool operator==(const OGRPoint &) const;
    bool operator!=(const GeoPoint &) const;
    bool operator!=(const OGRPoint &) const;
    bool operator<(const GeoPoint &) const;
    bool operator>(const GeoPoint &) const;
    bool static SortXPredicate(const GeoPoint *p1, const GeoPoint *p2) {return p1->_p.x < p2->_p.x;};
   
    void Merge(GeoPoint *);
    int NumOwners() const {return _owners.size();};
    const GeoRef * GetOwner(int index) const {return _owners[index];};
    bool IsDeleted() const {return _isDeleted;};
    GeoPoint * GetMasterPoint() {if (IsDeleted()) return _masterPoint; else return NULL;};
    void CompareOwners(const GeoPoint *p, bool &loss, bool &gain) const;
    void SortOwners();
    PointType type;
    OGRRawPoint _p;
private:
    bool _isDeleted;
    GeoPoint * _masterPoint;
    double _depth;
    OGRPoint *_ogrPointRef;
    vector<const GeoRef *> _owners;
};


#endif
