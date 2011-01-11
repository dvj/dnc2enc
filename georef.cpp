#include "georef.h"
#include "geosegment.h"

GeoRef::GeoRef(int id) {
    _id = id;
}

bool GeoRef::AddSegment(GeoSegment *gs, int winding) {
    GeoSegmentWinding *gsw = new GeoSegmentWinding(gs, winding);
    _segments.push_back(gsw);    
    return true;
}
