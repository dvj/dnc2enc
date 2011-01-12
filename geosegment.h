#ifndef _GEOSEGMENT_H_
#define _GEOSEGMENT_H_


#include <vector>
#include "geopoint.h"
using namespace std;

class GeoRef;

class GeoSegment {
public:
    GeoSegment(int );
    ~GeoSegment();
    bool Split(int index, GeoSegment *out);
    const GeoPoint *GetStartPoint() {return _startPoint;};
    const GeoPoint *GetEndPoint() {return _endPoint;};
    const GeoPoint *GetPoint(int index) const;
    int GetNumPoints() const { return _points.size()+2;};
    void AddStartPoint(GeoPoint *p, GeoRef *);
    void AddEndPoint(GeoPoint *p, GeoRef *);
    void AddPoint(GeoPoint *p, GeoRef *);
    const GeoRef* GetRef(int i)  const {return _references[i];};
    void FixPointReferences();
    int GetID() const {return _segmentID;};
private:
    vector<GeoRef *> _references;
    vector<GeoPoint *> _points;
    GeoPoint * _startPoint;
    GeoPoint * _endPoint;
    int _segmentID;
};


class GeoSegmentWinding {
public:
    GeoSegmentWinding(GeoSegment *, int winding);
    GeoSegment *GetSegment() const {return _segment;};
    int GetWindingDir() const {return _windingDirection;};
    int GetID() const {return _segment->GetID();};
    bool static IDSortPredicate(const GeoSegmentWinding *g1, const GeoSegmentWinding *g2) {return g1->GetID() < g2->GetID();};
    GeoSegment *_segment;
    int _windingDirection;
};



#endif
