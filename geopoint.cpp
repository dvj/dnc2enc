#include "geopoint.h"
#include <algorithm>


GeoPoint::GeoPoint() {

}


GeoPoint::GeoPoint(double x, double y, PointType pType, GeoRef *owner) {
    _p.x = x;
    _p.y = y;
    _depth = 0;
    type = pType;
    _ogrPointRef = NULL;
    _owners.push_back(owner);
    _isDeleted = false;
    _masterPoint = NULL;
    assigned = false;
    segmentID = -1;
}


GeoPoint::~GeoPoint() {


}


void GeoPoint::SetOGRPointReference(OGRPoint *pr) {
    _ogrPointRef = (OGRPoint*)pr->clone();
}

void GeoPoint::Merge(GeoPoint *gp) {
    for (int i = 0 ; i < gp->_owners.size() ; i++) {
        _owners.push_back(gp->_owners[i]);
    }
    if (gp->type == ConnectedPoint) 
        type = ConnectedPoint;
 
    gp->_isDeleted = true;
    gp->_masterPoint = this;
 
    //TODO - fix memory leak
    //*gp = *this;
}

void GeoPoint::SortOwners() {
    std::sort(_owners.begin(), _owners.end(), GeoRef::IDSortPredicate);
    for (vector<const GeoRef*>::iterator i = _owners.begin(); i != _owners.end(); i++) {
        while (i+1 != _owners.end() && (*(i+1))->GetID() == (*i)->GetID())
            _owners.erase(i+1);
    }
}

void GeoPoint::CompareOwners(const GeoPoint *p, bool &loss, bool &gain) const {

    vector<const GeoRef *>::const_iterator i = _owners.begin();
    vector<const GeoRef *>::const_iterator j = p->_owners.begin();
    gain = loss = false;
    while (i != _owners.end() && j != p->_owners.end()) {
        if ((*i)->GetID() == (*j)->GetID()) {
            i++;
            j++;
        } else if ((*i)->GetID() > (*j)->GetID()) {
            gain=true;
            j++;
        } else {
            loss=true;
            i++;
        }
    }
    if (i != _owners.end()) 
        loss = true;
    if (j != p->_owners.end())
        gain = true;
}




bool GeoPoint::operator==(const GeoPoint &p2) const {
    //printf("Comparing (%.8lf, %.8lf) to (%.8lf, %.8lf) ", this->_p.x, this->_p.y, p2._p.x, p2._p.y);
    return (fabs(this->_p.y-p2._p.y) < _Gtolerance &&
            fabs(this->_p.x-p2._p.x) < _Gtolerance );
}

bool GeoPoint::operator==(const OGRPoint &p2) const {
    return (fabs(this->_p.x-p2.getX()) < _Gtolerance &&
            fabs(this->_p.y-p2.getY()) < _Gtolerance);
}


bool GeoPoint::operator<(const GeoPoint &p2) const {
    return (this->_p.x-p2._p.x < -_Gtolerance);
}


bool GeoPoint::operator>(const GeoPoint &p2) const {
    return (this->_p.x-p2._p.x > _Gtolerance);
}


bool GeoPoint::operator!=(const GeoPoint &p2) const {
    return !(*this == p2);
}

bool GeoPoint::operator!=(const OGRPoint &p2) const {
    return !(*this == p2);
}
