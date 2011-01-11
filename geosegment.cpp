

#include "geosegment.h"

GeoSegment::GeoSegment() {
    _startPoint = NULL;
    _endPoint = NULL;
}

GeoSegment::~GeoSegment() {

}

bool GeoSegment::Split(int index, GeoSegment *out) {
    //printf("Splitting at (%d of %d)\n",index, GetNumPoints());
    if (index == 0) {
        return false;
    } else if (index > _points.size()) {
        return false;
    } else {
        vector<GeoPoint *>::iterator iter = _points.begin();
        if (index < _points.size()) {
            out->_points.assign(iter+index,_points.end());
            out->_startPoint = (*(iter+index-1));
            _points.erase(iter+index-1, _points.end());
        } else {
            out->_points.clear();
            out->_startPoint = _points[_points.size()-1];
            _points.pop_back();
        }
        out->_endPoint = _endPoint;
        _endPoint = out->_startPoint;
        out->_startPoint->type = ConnectedPoint;
        out->_endPoint->type = ConnectedPoint;
        _endPoint->type = ConnectedPoint;
    }
    out->_references.assign(_references.begin(), _references.end());
    return true;
}

void GeoSegment::AddPoint(GeoPoint *gp, GeoRef *gr) {
    _points.push_back(gp);
    _references.push_back(gr);
    if (gp->type != EdgePoint) { 
        fprintf(stderr,"Adding non-edge point to GeoSegment list\n");
    }
}

void GeoSegment::AddStartPoint(GeoPoint *gp, GeoRef *gr) {
    _startPoint = gp;
    _references.push_back(gr);
    if (gp->type != ConnectedPoint) { 
        fprintf(stderr,"Adding non-connected point as Start in GeoSegment list\n");
    } 

}


void GeoSegment::AddEndPoint(GeoPoint *gp, GeoRef *gr) {
    _endPoint = gp;
    _references.push_back(gr);
    if (gp->type != ConnectedPoint) { 
        fprintf(stderr,"Adding non-connected point as End in GeoSegment list\n");
    } 
}

void GeoSegment::FixPointReferences() {
    if (_startPoint->IsDeleted())
        _startPoint = _startPoint->GetMasterPoint();
    if (_endPoint->IsDeleted())
        _endPoint = _endPoint->GetMasterPoint();
    for (int i = 0 ; i < _points.size() ; i++) {
        if (_points[i]->IsDeleted())
            _points[i] = _points[i]->GetMasterPoint();
    }
    
    //now step through and remove any duplicates
    for (vector<GeoPoint *>::iterator i=_points.begin(); i != _points.end(); i++) {
        while ((i+1) != _points.end() && (*(i+1) == (*i))) {
            _points.erase(i+1);
        }
    }
}

const GeoPoint *GeoSegment::GetPoint(int index) const {
    GeoPoint *returnPoint = NULL;
    if (index==0) 
        returnPoint = _startPoint; 
    else if (index==(_points.size()+1)) 
        returnPoint =  _endPoint; 
    else 
        returnPoint = _points[index-1];

    return returnPoint;
        
}

GeoSegmentWinding::GeoSegmentWinding(GeoSegment *gs, int winding) {
    _segment = gs;
    _windingDirection = winding;
}
