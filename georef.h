#ifndef _GEOREF_H_
#define _GEOREF_H_

#include <vector>

using namespace std;

class GeoSegment;
class GeoSegmentWinding;

class GeoRef {
public:
    GeoRef(int );
    bool AddSegment(GeoSegment *, int winding);
    int GetNumSegments() const {return _segments.size();};
    GeoSegmentWinding* GetSegmentWinding(int index) {return _segments[index];};
    int GetID() const {return _id;};
    bool static IDSortPredicate(const GeoRef *g1, const GeoRef *g2) {return g1->GetID() < g2->GetID();};
    
    private:
    int _id;
    vector<GeoSegmentWinding *>_segments;
    
};


#endif

