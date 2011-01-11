#include <algorithm>
#include <list>
#include "geohandler.h"
#include "georef.h"
#include "mys57.h"
GeoHandler::GeoHandler(OGRDataSource *dat) {
    _gID = 0;
    _dataSource = dat;
    _tolerance = .00001;
    _pointListAllocSize = 400000;
    _pointListSize = 0;
    //_pointList = new GeoPoint*[_pointListAllocSize];
}


GeoHandler::~GeoHandler() {


}


int GeoHandler::LookupPoint(const GeoPoint *p) {
    int indexL = 0;
    int indexH = _pointList.size();
    int index = 0;
    while (indexL <= indexH) {
        index = (indexL+indexH)/2;
        if (*p < *_pointList[index])
            indexH = index-1;
        else if (*p > *_pointList[index])
            indexL = index+1;
        else {
            //becasue there may be repeats, step down until we no longer match
            if (index == 0) return index;
            while (fabs(p->_p.x - _pointList[index-1]->_p.x) < _Gtolerance) {
                if (index == 0) return index;
                index--;
            }
            return index;
        }
    }
    //fprintf(stderr, "Didn't find point %.5lf!!!\n", p->_p.x);
    return index;
}

GeoPoint * GeoHandler::CreatePoint(OGRPoint *ogrpt, PointType t, GeoRef *gr) {
    GeoPoint *p = new GeoPoint(ogrpt->getX(),ogrpt->getY(), t, gr);
    p->SetOGRPointReference(ogrpt);
    _pointList.push_back(p);
    return p;
}


int GeoHandler::AddLineString(const OGRLineString *ls, GeoSegment *gs, GeoRef *gr) {
    int numPoints = ls->getNumPoints();
    OGRPoint *p = new OGRPoint();
    GeoPoint *gp = NULL;
    ls->getPoint(0,p);
    gp = CreatePoint(p, ConnectedPoint, gr);
    gs->AddStartPoint(gp,gr);
    ls->getPoint(numPoints-1,p);
    if (*gp != *p) {
        //The Start and End point are NOT the same
        gp = CreatePoint(p, ConnectedPoint, gr);
    }
    gs->AddEndPoint(gp,gr);

    // now add all the middle points as EdgePoints
    for (int i = 1 ; i < numPoints-1 ; i++) {
        ls->getPoint(i,p);
        gp = CreatePoint(p, EdgePoint, gr);
        gs->AddPoint(gp,gr);
    }
    return 0;
}

GeoRef* GeoHandler::ProcessFeature(OGRFeature *feature) {
    OGRGeometry *fgeo = feature->GetGeometryRef();
    OGRwkbGeometryType geoType = fgeo->getGeometryType();
    int featureID = _gID;//feature->GetFieldAsInteger(feature->GetFieldIndex("id"));

    // Dont need point or multipoint, becasue they will always be isolated
    if (geoType == wkbPoint) {
        //TODO - Maybe add a georef here so we have everything encapsulated??

        //AddPoint((OGRPoint *)fgeo, IsolatedPoint);
    } else if (geoType == wkbMultiPoint) {
        /*OGRMultiPoint *ogrpts = (OGRMultiPoint *) fgeo;
        int numPoints =ogrpts->getNumGeometries();
        for (int i = 0 ; i < numPoints ; i++) {
            AddPoint((OGRPoint*)ogrpts->getGeometryRef(i), IsolatedPoint);
        }*/
    } else if (geoType == wkbLineString) {
        GeoRef *gr = new GeoRef(featureID); 
        OGRLineString *ls = (OGRLineString *) fgeo;
        GeoSegment *gs = new GeoSegment();
        AddLineString(ls, gs, gr);
        gr->AddSegment(gs,-1);
        _refList.push_back(gr);
        return gr;
    } else if (geoType ==  wkbPolygon) {
        GeoRef *gr = new GeoRef(featureID);
        OGRPolygon *poly = (OGRPolygon *)fgeo;
        poly->closeRings();
        OGRLinearRing *ogrRing;
        GeoSegment *gs = new GeoSegment();
        ogrRing = (OGRLinearRing *)poly->getExteriorRing();
        AddLineString((OGRLineString *)ogrRing,gs, gr);
        int cw = 0;
        if (ogrRing->isClockwise()) cw = 0; else cw = 1;
        gr->AddSegment(gs,cw);
        int iRings = poly->getNumInteriorRings();
        for (int i = 0 ; i < iRings ; i++ ) {
            gs = new GeoSegment();
            ogrRing = (OGRLinearRing *)poly->getInteriorRing(i);
            AddLineString((OGRLineString *)ogrRing,gs, gr);
            if (ogrRing->isClockwise()) cw = 0; else cw = 1;
            gr->AddSegment(gs,cw);
        }
        _refList.push_back(gr);
        return gr;
    } else {
        fprintf(stderr, "Unhandled Geometry in ProcessFeature: %s\n", fgeo->getGeometryName());
        return(0);
    }

    return 0;
}

int GeoHandler::ReadGeometry() {
    int layers = _dataSource->GetLayerCount();
    OGRLayer *layer;
    for (int l = 0 ; l< layers; l++) {
        layer = _dataSource->GetLayer(l);
        if (layer == NULL || layer->GetFeatureCount() == 0) continue;
        
        OGRFeature *feature;
        layer->ResetReading();
        while( (feature = layer->GetNextFeature()) != NULL ) {
            _gID++;
            ProcessFeature(feature);
        }
    }

    /* 
    for (unsigned i = 0 ; i < _refList.size() ; i++ ) {
        GeoRef *gr = _refList[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            GeoSegmentWinding *gsw = gr->GetSegmentWinding(j);
            GeoSegment *gs = gsw->GetSegment();
            vector<int> *ownerIDs = new vector<int>[gs->GetNumPoints()];
            for (int p = 0 ; p < gs->GetNumPoints(); p++) {
                const GeoPoint *gp = gs->GetPoint(p);
                
                if (gp->IsDeleted()) printf("*");
                else if (gp->type == ConnectedPoint) printf("+");
                else printf(" ");
                //int idx = LookupPoint(gp);
                // printf("Found %.5lf at index %d (%.5lf)\n",gp->_p.x, idx, _pointList[idx]->_p.x);
                printf("%d %x %d (%.5lf %.5lf):",gs->GetRef(0)->GetID(), gp, gsw->GetWindingDir(), gp->_p.x, gp->_p.y);
                for (int owner=0; owner < gp->NumOwners(); owner++) {
                    ownerIDs[p].push_back(gp->GetOwner(owner)->GetID());
                    printf("%d ",gp->GetOwner(owner)->GetID());
                }
              
                printf("\n");
            }
            printf("---------\n");
        }
    }
    */




    printf("Reflist.size: %d\n",(int) _refList.size());
    printf("PointList.size: %d\n",(int)_pointList.size());
    
    GeoRef *gr = NULL;
    int totalGeoCount = 0;
    for (unsigned int i = 0 ; i < _refList.size(); i++ ) {
        gr = _refList[i];
        totalGeoCount+= gr->GetNumSegments();
    }
    printf("GeoCount: %d\n",totalGeoCount);
    std::sort(_pointList.begin(), _pointList.end(), GeoPoint::SortXPredicate);
    //copy the vector to a list
    list<GeoPoint *> plist;
    int dbgRefCount = 0;
    int dbgSize = _pointList.size();
    for (int i = 0 ; i < _pointList.size(); i++ ) {
        plist.push_back(_pointList[i]);
        dbgRefCount += _pointList[i]->NumOwners();
    }
    //go through entire list and merge like points.
    list<GeoPoint *>::iterator idx;
    for (list<GeoPoint *>::iterator i = plist.begin() ; i != plist.end(); i++ ) {
        idx = i;
        idx++;
        GeoPoint *p = (*i);
        while (idx != plist.end() && fabs(p->_p.x - (*idx)->_p.x) < _Gtolerance) {
            if ((*p) == **idx) {
                p->Merge(*idx);                
                idx = plist.erase(idx);
            } else {
                idx++;
            }
        }
    }
/*
    //  sort the owners for all points
    for (int i = 0 ; i < _pointList.size(); i++ ) {
        _pointList[i]->SortOwners();
    }
*/              
    _pointList.clear();
    int dbgRefCount2 = 0;
    //now copy the list back to the vector,
    for (list<GeoPoint *>::iterator i = plist.begin() ; i != plist.end(); i++ ) {
        _pointList.push_back(*i);
        (*i)->SortOwners();
        //printf("%.8lf %.8lf %d\n", (*i)->_p.x, (*i)->_p.y, (*i)->NumOwners());
        dbgRefCount2 += (*i)->NumOwners();
    }

    printf("Size - before: %d   after: %d\n",dbgSize, _pointList.size());
    printf("Reference Counts - before: %d   after: %d\n",dbgRefCount, dbgRefCount2);
    ///For each geometry segment, walk the points and build a list of others that share that point
    //Anytime someone joins or leaves, make it a ConnectedNode, and split the respective geometries
    for (unsigned i = 0 ; i < _refList.size() ; i++ ) {
        GeoRef *gr = _refList[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            GeoSegmentWinding *gsw = gr->GetSegmentWinding(j);
            GeoSegment *gs = gsw->GetSegment();
            gs->FixPointReferences();
            vector<int> *ownerIDs = new vector<int>[gs->GetNumPoints()];
            bool loss, gain;
            for (int p = 0 ; p < gs->GetNumPoints(); p++) {
                const GeoPoint *gp = gs->GetPoint(p);
                if (p != gs->GetNumPoints()-1) {
                    const GeoPoint *gp2 = gs->GetPoint(p+1);
                    gp->CompareOwners(gp2, loss, gain);
                    if (loss) {
                        GeoSegment* gs2 = new GeoSegment();
                        if (gs->Split(p, gs2))
                            gr->AddSegment(gs2, gsw->GetWindingDir());
                    }
                    if (gain) {
                        GeoSegment* gs2 = new GeoSegment();
                        if (gs->Split(p+1, gs2)) {
                            gr->AddSegment(gs2, gsw->GetWindingDir());
                        }
                    }
                } else {
                    loss = gain = false;
                }
                    
                /*
                if (gp->IsDeleted()) printf("*");
                else if (gp->type == ConnectedPoint) printf("+");
                else printf(" ");
                //int idx = LookupPoint(gp);
                // printf("Found %.5lf at index %d (%.5lf)\n",gp->_p.x, idx, _pointList[idx]->_p.x);
                printf("%d %x %d %d %d (%.5lf %.5lf):",gs->GetRef(0)->GetID(), gp, gsw->GetWindingDir(), loss, gain, gp->_p.x, gp->_p.y);
                */
                for (int owner=0; owner < gp->NumOwners(); owner++) {
                    ownerIDs[p].push_back(gp->GetOwner(owner)->GetID());
                    //printf("%d ",gp->GetOwner(owner)->GetID());
                }
              
                //printf("\n");
            }
            //printf("---------\n");
        }
    }
    

     
    for (unsigned i = 0 ; i < _refList.size() ; i++ ) {
        GeoRef *gr = _refList[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            GeoSegmentWinding *gsw = gr->GetSegmentWinding(j);
            GeoSegment *gs = gsw->GetSegment();
            vector<int> *ownerIDs = new vector<int>[gs->GetNumPoints()];
            for (int p = 0 ; p < gs->GetNumPoints(); p++) {
                const GeoPoint *gp = gs->GetPoint(p);
                
                if (gp->IsDeleted()) printf("*");
                else if (gp->type == ConnectedPoint) printf("+");
                else printf(" ");
                // printf("Found %.5lf at index %d (%.5lf)\n",gp->_p.x, idx, _pointList[idx]->_p.x);
                printf("%d %x %d (%.5lf %.5lf):",gs->GetRef(0)->GetID(), gp, gsw->GetWindingDir(), gp->_p.x, gp->_p.y);
                for (int owner=0; owner < gp->NumOwners(); owner++) {
                    printf("%d ",gp->GetOwner(owner)->GetID());
                }
              
                printf("\n");
            }
            printf("---------\n");
        }
    }

    //std::sort(_refList.begin(), _refList.end(), GeoRef::IDSortPredicate);
    return 0;
}


int GeoHandler::HandleGeometry(OGRFeature *poFeature) {
    int madeGeoType = 0;
    OGRGeometry *poGeometry;
    poGeometry = poFeature->GetGeometryRef();
    if( poGeometry == NULL) {
        return 0;
    }
    OGRFeature *poFeatureO = NULL;
    OGRLayer *encLayer;
    if (wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
        encLayer = _dataSource->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        madeGeoType = RCNM_VI;
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbLineString ) {
        OGRLineString *line = (OGRLineString *)poGeometry;
        madeGeoType = RCNM_VE;
        OGRPoint *startPoint = new OGRPoint();
        line->StartPoint(startPoint); 
        OGRPoint *endPoint = new OGRPoint();
        line->EndPoint(endPoint); 
        int startNode = MakeConnectedNode(startPoint);
        int endNode = MakeConnectedNode(endPoint);
        encLayer = _dataSource->GetLayer(2);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());                
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),madeGeoType);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_0"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_0"),startNode);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_1"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_1"),endNode);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_0"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_0"),255);
        
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) {
        OGRPolygon *poly = (OGRPolygon *) poGeometry;
        OGRLineString *line = (OGRLineString *) poly->getExteriorRing();
       
        OGRPoint *startPoint = new OGRPoint();
        line->StartPoint(startPoint); 
        /*OGRPoint *endPoint = new OGRPoint();
        line->EndPoint(endPoint); 
        */
        int startNode = MakeConnectedNode(startPoint);
        //int endNode = MakeConnectedNode(endPoint);
       
        int rings = poly->getNumInteriorRings();
        //TODO - something intelligent with the rings
        /*
        if (rings > 0) {
            printf("Found %d interior rings\n",rings);
            for (int r = 0;r<rings;r++) {
                OGRLinearRing *ring = poly->getInteriorRing(r);
                printf("  Ring %d is wound: %d, has %d nodes\n",r,ring->isClockwise(), ring->getNumPoints());
            }
            }*/
        encLayer = _dataSource->GetLayer(2);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VE);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_0"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_0"),startNode);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_1"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_1"),startNode);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_0"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_1"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_1"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_1"),2);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_1"),255);        
        poGeometry = line;
        madeGeoType = RCNM_VE;   
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPoint) {
        encLayer = _dataSource->GetLayer(0);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        madeGeoType = RCNM_VI;
    } else {
        printf("OTHER GEOMETRY OF TYPE %d\n",poGeometry->getGeometryType());
        encLayer = _dataSource->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        madeGeoType = RCNM_VI;
    }
    poFeatureO->SetGeometry( poGeometry );
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),poFeature->GetFieldAsInteger(poFeature->GetFieldIndex("id")));
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),_gID++);
    if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in shapefile.\n" );
        return 0;
    }
    //poFeatureO->DumpReadable(0,0);
    OGRFeature::DestroyFeature(poFeatureO);
    return madeGeoType;
}



int GeoHandler::MakeConnectedNode(OGRPoint *p) {
     OGRLayer *encLayer =  _dataSource->GetLayer(1);
     OGRFeature *poFeature =OGRFeature::CreateFeature(encLayer->GetLayerDefn());
     poFeature->SetField(poFeature->GetFieldIndex("RCNM"),RCNM_VC);
     poFeature->SetGeometry(p);
     poFeature->SetField(poFeature->GetFieldIndex("RUIN"),1);
     poFeature->SetField(poFeature->GetFieldIndex("RCID"),_gID++);
     if( encLayer->CreateFeature( poFeature ) != OGRERR_NONE )
     {
         printf( "Failed to create feature in shapefile.\n" );
         return -1;
     }
     return _gID-1;
}

int GeoHandler::GetGID() {
    return _gID;
}
