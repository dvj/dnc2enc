#include <algorithm>
#include <list>
#include "geohandler.h"
#include "georef.h"
#include "mys57.h"
#include "buildtables.h"
GeoHandler::GeoHandler(OGRDataSource *datin, OGRDataSource *datout) {
    _globalFeatureID = 0;
    _globalSegmentID = 0;
    _dataSource = datin;
    _outputSource = datout;
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


GeoRef * GeoHandler::LookupGeoRef(int id) {
    int indexL = 0;
    int indexH = _refList.size();
    int index = 0;
    while (indexL <= indexH) {
        index = (indexL+indexH)/2;
        if (id < _refList[index]->GetID())
            indexH = index-1;
        else if (id > _refList[index]->GetID())
            indexL = index+1;
        else {
            return _refList[index];
        }
    }
    fprintf(stderr, "Didn't find point ID %d!!!\n", id);
    return _refList[index];
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
    int featureID = _globalFeatureID;//feature->GetFieldAsInteger(feature->GetFieldIndex("id"));

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
        GeoSegment *gs = new GeoSegment(_globalSegmentID++);
        AddLineString(ls, gs, gr);
        gr->AddSegment(gs,-1);
        _refList.push_back(gr);
        return gr;
    } else if (geoType ==  wkbPolygon) {
        GeoRef *gr = new GeoRef(featureID);
        OGRPolygon *poly = (OGRPolygon *)fgeo;
        poly->closeRings();
        OGRLinearRing *ogrRing;
        GeoSegment *gs = new GeoSegment(_globalSegmentID++);
        ogrRing = (OGRLinearRing *)poly->getExteriorRing();
        AddLineString((OGRLineString *)ogrRing,gs, gr);
        int cw = 0;
        if (ogrRing->isClockwise()) cw = 0; else cw = 1;
        gs->originalWindingDir = cw;
        gr->AddSegment(gs,cw);
        int iRings = poly->getNumInteriorRings();
        for (int i = 0 ; i < iRings ; i++ ) {
            gs = new GeoSegment(_globalSegmentID++);
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


int GeoHandler::WriteVectorRecords(vector<GeoRef *> vrecs) {

    //write out the vector records
    for (unsigned i = 0 ; i < vrecs.size() ; i++ ) {
        GeoRef *gr = vrecs[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            GeoSegmentWinding *gsw = gr->GetSegmentWinding(j);
            GeoSegment *gs = gsw->GetSegment();
            if (!gs->assigned) {
                gs->assigned = true;
                int startNode = -1;
                int endNode = -1;
                if (gs->GetStartPoint()->assigned == false) {
                    startNode = MakeConnectedNode(gs->GetStartPoint()->GetOGRPointRef());
                    gs->GetStartPoint()->segmentID = startNode;
                    gs->GetStartPoint()->assigned = true;
                } else {
                    startNode = gs->GetStartPoint()->segmentID;
                }
                if (gs->GetEndPoint()->assigned == false) {
                    endNode = MakeConnectedNode(gs->GetEndPoint()->GetOGRPointRef());
                    gs->GetEndPoint()->segmentID = endNode;
                    gs->GetEndPoint()->assigned = true;
                } else {
                    endNode = gs->GetEndPoint()->segmentID;
                }
                
                OGRLineString *line = new OGRLineString();
                for (int i = 0 ; i < gs->GetNumPoints() ; i++) {
                    line->addPoint((OGRPoint*)gs->GetPoint(i)->GetOGRPointRef()->clone());
                }
                OGRLayer *encLayer = _outputSource->GetLayer(2);
                OGRFeature *poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());                
                poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VE);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_0"),RCNM_VC);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_0"),startNode);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_1"),RCNM_VC);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_1"),endNode);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_0"),255);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_0"),255);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_0"),1);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_0"),255);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_1"),255);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_1"),255);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_1"),2);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_1"),255);
                poFeatureO->SetGeometry( line );
                poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
                poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),gs->GetID());
                if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
                {
                    printf( "Failed to create feature in shapefile.\n" );
                }
                //poFeatureO->DumpReadable(0,0);
                OGRFeature::DestroyFeature(poFeatureO);
            }
        }
    }
}


void GeoHandler::ReProcessFeature(OGRFeature *feature) {
    OGRGeometry *fgeo = feature->GetGeometryRef();
    OGRwkbGeometryType geoType = fgeo->getGeometryType();
    int featureID = _globalFeatureID;//feature->GetFieldAsInteger(feature->GetFieldIndex("id"));
    OGRFeature *poFeatureO = NULL;
    OGRLayer *encLayer;

    int geometryType = 0;
    const char *f_code;
    char *sLookup = new char[64];
    
    if (feature->GetFieldIndex("f_code") != -1) {
        f_code = feature->GetFieldAsString(feature->GetFieldIndex("f_code"));
        strcpy(sLookup,f_code);
    } else {
        //printf("No f_code\n");
        strcpy(sLookup,"");
    }

    if (geoType == wkbPoint) {
        encLayer = _outputSource->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        poFeatureO->SetGeometry( fgeo );
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),_globalSegmentID++);
        if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in shapefile.\n" );
            return;
        }

        geometryType = 1;
        strcat(sLookup,"_Point");

        //AddPoint((OGRPoint *)fgeo, IsolatedPoint);
    } else if (geoType == wkbMultiPoint) {
        encLayer = _outputSource->GetLayer(0);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        poFeatureO->SetGeometry( fgeo );
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),_globalSegmentID++);
        if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in shapefile.\n" );
            return;
        }

        geometryType = 1;
        strcat(sLookup,"_Point");

        /*OGRMultiPoint *ogrpts = (OGRMultiPoint *) fgeo;
        int numPoints =ogrpts->getNumGeometries();
        for (int i = 0 ; i < numPoints ; i++) {
            AddPoint((OGRPoint*)ogrpts->getGeometryRef(i), IsolatedPoint);
        }*/
    } else if (geoType == wkbLineString) {
        geometryType = 2;
        strcat(sLookup,"_Line");
    } else if (geoType ==  wkbPolygon) {
          geometryType = 3;
          strcat(sLookup,"_Area");
          
    } else {
        fprintf(stderr, "Unhandled Geometry in ProcessFeature: %s\n", fgeo->getGeometryName());
        return;
    }


    int ENCLayerNum = _lookuptable[sLookup];
    if (ENCLayerNum == 0  || ENCLayerNum == -1) {
        //printf("No available layer for %s, %s\n",poLayer->GetLayerDefn()->GetName(), sLookup);
        return;
    }
    
    encLayer = _outputSource->GetLayer(ENCLayerNum+3);
    
    poFeatureO = new OGRFeature(encLayer->GetLayerDefn());
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),_globalSegmentID++);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("PRIM"),geometryType);
            
    // Set the "Skin of Earth" flag
    if (ENCLayerNum == 42 || ENCLayerNum == 46 || ENCLayerNum == 57 ||
        ENCLayerNum == 65 || ENCLayerNum == 71 || ENCLayerNum == 95 || ENCLayerNum == 154) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("GRUP"),1); 
    } else {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("GRUP"),2); 
    }

    poFeatureO->SetField(poFeatureO->GetFieldIndex("OBJL"),ENCLayerNum);
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RVER"),1);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("AGEN"),40); //FIXME - find a good agency
    // poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),poFeature->GetFieldAsInteger(poFeature->GetFieldIndex("id")));
    
    poFeatureO->SetField(poFeatureO->GetFieldIndex("FIDN"),ENCLayerNum);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("FIDS"),feature->GetFieldAsInteger(feature->GetFieldIndex("id")));
    if ((geoType == wkbPoint || geoType == wkbMultiPoint)) {
        
        int *gl = new int[1];
        gl[0] = RCNM_VI;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM"),1,gl);
        gl[0] = _globalSegmentID-2;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID"),1,gl);
        gl[0] = 1;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT"),1,gl);
        gl[0] = 1;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG"),1,gl);
        gl[0] = 2;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK"),1,gl);
                
        //poFeatureO->SetField(poFeatureO->GetFieldIndex("LNAM_REFS"),
    } else {
        //Lookup georef
        GeoRef *gr = LookupGeoRef(featureID);
        int numSegs = gr->GetNumSegments();
        int *rcnm = new int[numSegs];
        int *rcid = new int[numSegs];
        int *ornt = new int[numSegs];
        int *usag = new int[numSegs];
        int *mask = new int[numSegs];
        GeoSegmentWinding *gsw = NULL;
        for (int i = 0 ; i < numSegs ; i++) {
            gsw = gr->GetSegmentWinding(i);
            rcnm[i] = RCNM_VE;
            rcid[i] = gsw->GetID();
            ornt[i] = gsw->GetWindingDir();
            usag[i] = gsw->GetSegment()->originalWindingDir+1;
            mask[i] = 2;
        }
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM"),numSegs,rcnm);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID"),numSegs,rcid);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT"),numSegs,ornt);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG"),numSegs,usag);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK"),numSegs,mask);
    }
    if (feature->GetFieldIndex("nam") != -1 && poFeatureO->GetFieldIndex("OBJNAM") != -1) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("OBJNAM"), feature->GetFieldAsString(feature->GetFieldIndex("nam")));
    }

    //poFeatureO->DumpReadable(0,0);
    encLayer = _outputSource->GetLayer(poFeatureO->GetFieldAsInteger("OBJL")+3);
    if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in shapefile.\n" );   
    }


    return;
}


void GeoHandler::WriteFeatureRecords() {

    int layers = _dataSource->GetLayerCount();
    OGRLayer *layer;
    BuildTables(&_lookuptable);

    //reset the featureID back to 0, so the number matches when we re-read the file
    _globalFeatureID = 0;

    //sort the georefs so we can look them up (from the FeatureID)
    std::sort(_refList.begin(), _refList.end(), GeoRef::IDSortPredicate);

    for (int l = 0 ; l< layers; l++) {
        layer = _dataSource->GetLayer(l);
        if (layer == NULL || layer->GetFeatureCount() == 0) continue;
        
        OGRFeature *feature;
        layer->ResetReading();
        while( (feature = layer->GetNextFeature()) != NULL ) {
            _globalFeatureID++;
            ReProcessFeature(feature);
        }
    }
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
            _globalFeatureID++;
            ProcessFeature(feature);
        }
    }

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
    plist.assign(_pointList.begin(),_pointList.end());
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
          
    _pointList.clear();
    int dbgRefCount2 = 0;
    //now copy the list back to the vector, and sort the owners while we're at it
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
            bool loss, gain;
            for (int p = 0 ; p < gs->GetNumPoints()-1; p++) {
                const GeoPoint *gp = gs->GetPoint(p);
                const GeoPoint *gp2 = gs->GetPoint(p+1);
                gp->CompareOwners(gp2, loss, gain);
                if (loss && p > 0) {
                    GeoSegment* gs2 = new GeoSegment(_globalSegmentID++);
                    if (gs->Split(p, gs2))
                        gr->AddSegment(gs2, gsw->GetWindingDir());
                    else delete gs2;
                }
                if (gain) {
                    GeoSegment* gs2 = new GeoSegment(_globalSegmentID++);
                    if (gs->Split(p+1, gs2)) {
                        gr->AddSegment(gs2, gsw->GetWindingDir());
                    } else delete gs2;
                }
            }
        }
    }
    
    // now go through the segments once more, and split on any interior connected points     
    for (unsigned i = 0 ; i < _refList.size() ; i++ ) {
        GeoRef *gr = _refList[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            GeoSegmentWinding *gsw = gr->GetSegmentWinding(j);
            GeoSegment *gs = gsw->GetSegment();
            for (int p = 1 ; p < gs->GetNumPoints()-1; p++) {
                const GeoPoint *gp = gs->GetPoint(p);
                if (gp->type == ConnectedPoint) {
                    GeoSegment *gs2 = new GeoSegment(_globalSegmentID++);
                    if (gs->Split(p, gs2))
                        gr->AddSegment(gs2, gsw->GetWindingDir());
                    else delete gs2;
                }
            }
        }
    }
   
    GeoRef *gr2;
    GeoSegment *gs, *gs2;
    GeoSegmentWinding *gsw, *gsw2;
    //trim down segments. this is n^2, but hopefully n isn't too big
    int refListSize = _refList.size();
    for (unsigned i = 0 ; i < refListSize ; i++ ) {
        printf("%d of %d\n",i, refListSize);
        gr = _refList[i];
        for (int j = 0 ; j < gr->GetNumSegments(); j++) {
            gsw = gr->GetSegmentWinding(j);
            gs = gsw->GetSegment();
            for (unsigned i2 = i+1 ; i2 < refListSize ; i2++ ) {
                gr2 = _refList[i2];
                int numSegs = gr2->GetNumSegments();
                for (int j2 = 0 ; j2 < numSegs; j2++) {
                    gsw2 = gr2->GetSegmentWinding(j2);
                    gs2 = gsw2->GetSegment();
                    if (gs != gs2 && gs->GetNumPoints() == gs2->GetNumPoints()) {
                        if (gs->GetStartPoint() == gs2->GetStartPoint() && gs->GetEndPoint() == gs2->GetEndPoint() && gs->GetPoint(1) == gs2->GetPoint(1)) {
                            //printf("FW replaced %d with %d\n",gsw2->GetID(), gsw->GetID());
                            gsw2->_segment = gs;
                            gsw2->_windingDirection = gsw->_windingDirection;
                            // delete gs2;
                        } else if (gs->GetStartPoint() == gs2->GetEndPoint() && gs->GetEndPoint() == gs2->GetStartPoint()  && gs->GetPoint(1) == gs2->GetPoint(gs2->GetNumPoints()-2)) {
                            //printf("BW replaced %d with %d\n",gsw2->GetID(), gsw->GetID());
                            gsw2->_segment = gs;
                            gsw2->_windingDirection = !gsw->_windingDirection;
                            // delete gs2;
                        }
                    }
                }
            }
        }
    }
    printf("Writing Vector records...");
    fflush(stdout);
    WriteVectorRecords(_refList);
    printf("Done\nWriting Feature Records...");
    fflush(stdout);
    WriteFeatureRecords();
    printf("Done\n");
    /*
    //debug printing
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
                printf("%d %d (%.5lf %.5lf):",gs->GetID(), gsw->GetWindingDir(), gp->_p.x, gp->_p.y);
                for (int owner=0; owner < gp->NumOwners(); owner++) {
                    printf("%d ",gp->GetOwner(owner)->GetID());
                }          
                printf("\n");
            }
            if (j< gr->GetNumSegments()-1) printf("\n");
        }
        printf("----------------\n");
    }
    */
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
        encLayer = _outputSource->GetLayer(0);
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
        encLayer = _outputSource->GetLayer(2);
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
        encLayer = _outputSource->GetLayer(2);
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
        encLayer = _outputSource->GetLayer(0);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        madeGeoType = RCNM_VI;
    } else {
        printf("OTHER GEOMETRY OF TYPE %d\n",poGeometry->getGeometryType());
        encLayer = _outputSource->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        madeGeoType = RCNM_VI;
    }
    poFeatureO->SetGeometry( poGeometry );
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),_globalFeatureID++);
    if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in shapefile.\n" );
        return 0;
    }
    //poFeatureO->DumpReadable(0,0);
    OGRFeature::DestroyFeature(poFeatureO);
    return madeGeoType;
}



int GeoHandler::MakeConnectedNode(const OGRPoint *p) {
     OGRLayer *encLayer =  _outputSource->GetLayer(1);
     OGRFeature *poFeature =OGRFeature::CreateFeature(encLayer->GetLayerDefn());
     poFeature->SetField(poFeature->GetFieldIndex("RCNM"),RCNM_VC);
     poFeature->SetGeometry((OGRPoint *)p->clone());
     poFeature->SetField(poFeature->GetFieldIndex("RUIN"),1);
     poFeature->SetField(poFeature->GetFieldIndex("RCID"),_globalSegmentID++);
     if( encLayer->CreateFeature( poFeature ) != OGRERR_NONE )
     {
         printf( "Failed to create feature in shapefile.\n" );
         return -1;
     }
     return _globalSegmentID-1;
}

int GeoHandler::GetGID() {
    return _globalFeatureID;
}
