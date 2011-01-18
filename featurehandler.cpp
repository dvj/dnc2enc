#include <algorithm>
#include "mys57.h"
#include "featurehandler.h"
#include "georef.h"
#include "geosegment.h"
#include "buildtables.h"

FeatureHandler::FeatureHandler(OGRDataSource *indat, OGRDataSource *outdat) {
    
    _dataSource = indat;
    _outputSource = outdat;
}


FeatureHandler::~FeatureHandler() {


}


GeoRef * FeatureHandler::LookupGeoRef(int id) {
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


void FeatureHandler::ReProcessFeature(OGRFeature *feature, int featureID) {
    OGRGeometry *fgeo = feature->GetGeometryRef();
    OGRwkbGeometryType geoType = fgeo->getGeometryType();
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
        
        if (feature->GetFieldIndex("hdp") != -1) {
            OGRPoint *p = (OGRPoint *)fgeo;
            double d = feature->GetFieldAsDouble("hdp"); //TODO - check the units of this
            if (!isnan(d)) {
                p->setZ(-d);
            }
            poFeatureO->SetGeometry(p);
        } else {
            poFeatureO->SetGeometry( fgeo );
        }
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
        //printf("No available layer for %s, %s, %s\n",f_code,feature->GetDefnRef()->GetName(), sLookup);
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
    if (feature->GetFieldIndex("txt") != -1 && poFeatureO->GetFieldIndex("INFORM") != -1) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("INFORM"), feature->GetFieldAsString(feature->GetFieldIndex("txt")));
    } 
    if (feature->GetFieldIndex("cvl") != -1 && poFeatureO->GetFieldIndex("DRVAL1") != -1) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("DRVAL1"), feature->GetFieldAsDouble(feature->GetFieldIndex("cvl")));
    }
    if (feature->GetFieldIndex("cvh") != -1 && poFeatureO->GetFieldIndex("DRVAL2") != -1) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("DRVAL2"), feature->GetFieldAsDouble(feature->GetFieldIndex("cvh")));
    }
    if (feature->GetFieldIndex("hdh") != -1 && poFeatureO->GetFieldIndex("DRVAL1") != -1) {
        poFeatureO->SetField(poFeatureO->GetFieldIndex("DRVAL1"), -1.*feature->GetFieldAsDouble(feature->GetFieldIndex("hdh")));
        double hdp = feature->GetFieldAsDouble(feature->GetFieldIndex("hdp"));
        if (isnan(hdp)) hdp = 0;
        poFeatureO->SetField(poFeatureO->GetFieldIndex("DRVAL2"), -1.*hdp);
    }
    if (feature->GetFieldIndex("eol") != -1 && poFeatureO->GetFieldIndex("VERLEN") != -1) {
        double eol = feature->GetFieldAsDouble(feature->GetFieldIndex("eol"));
        if (fabs(eol) < 30000) //null is sometimes indicated as -32768
            poFeatureO->SetField(poFeatureO->GetFieldIndex("VERLEN"), eol);
    }
    if (feature->GetFieldIndex("ias") != -1 && poFeatureO->GetFieldIndex("CATTSS") != -1) {
        int ias = feature->GetFieldAsInteger(feature->GetFieldIndex("ias"));
        poFeatureO->SetField(poFeatureO->GetFieldIndex("CATTSS"), ias);
    }

    if (feature->GetFieldIndex("ccc") != -1 && poFeatureO->GetFieldIndex("COLOUR") != -1) {
        int ccc = feature->GetFieldAsInteger(feature->GetFieldIndex("ccc"));
        char *colour = new char[32];
        strcpy(colour,"");
        switch (ccc) 
        {
            case 1: strcpy(colour,"2"); break;
            case 2: strcpy(colour,"5"); break;
            case 3: strcpy(colour,"8"); break;
            case 4: strcpy(colour,"7"); break;
            case 5: strcpy(colour,"4"); break;
            case 9: strcpy(colour,"11"); break;
            case 12: strcpy(colour,"3"); break;
            case 14: strcpy(colour,"10"); break;
            case 15: strcpy(colour,"1"); break;
            case 19: strcpy(colour,"6"); break;
            default: break;
        }
        if (feature->GetFieldIndex("col")) {
            const char *col = feature->GetFieldAsString(feature->GetFieldIndex("col"));
            int index = -1;
            int litchr = -1;
            if (strncasecmp(col,"Fl",2) == 0) {
                index = 3;
                litchr = 2;
            } else if (strncasecmp(col,"Iso",3) == 0) {
                index = 4;
                litchr = 7;
            } else if (strncasecmp(col,"F",1) == 0) {
                index = 2;
                litchr = 1;
            } else if (strncasecmp(col,"Q",1) == 0) {
                index = 2;
                litchr = 4;
            } else if (strncasecmp(col,"Oc",1) == 0) {
                index = 3;
                litchr = 8;
            }
            if (index > 0) {
                switch (col[index]) 
                {
                    case 'W': strcpy(colour, "1"); break;
                    case 'L': strcpy(colour, "2"); break;
                    case 'R': strcpy(colour, "3"); break;
                    case 'G': strcpy(colour, "4"); break;
                    case 'B': strcpy(colour, "5"); break;
                    case 'Y': strcpy(colour, "6"); break;
                }
            }
            if (litchr > 0 && poFeatureO->GetFieldIndex("LITCHR") != -1) {
                poFeatureO->SetField(poFeatureO->GetFieldIndex("LITCHR"), litchr);
            }
        }
        poFeatureO->SetField(poFeatureO->GetFieldIndex("COLOUR"), colour);
        if (feature->GetFieldIndex("per") != -1 && poFeatureO->GetFieldIndex("SIGPER") != -1) {
            poFeatureO->SetField(poFeatureO->GetFieldIndex("SIGPER"), feature->GetFieldAsDouble(feature->GetFieldIndex("per")));
        }
    }
    if (feature->GetFieldIndex("ssc") != -1 && poFeatureO->GetFieldIndex("BOYSHP") != -1) {
        int ssc = feature->GetFieldAsInteger(feature->GetFieldIndex("ssc"));
        int boyshp = 0;
        switch (ssc) 
        {
            case 1: boyshp = 6; break;
            case 6: boyshp = 1; break;
            case 7: boyshp = 2; break;
            case 10: boyshp = 4; break;
            case 16: boyshp = 5; break;
            case 17: boyshp = 3; break;
            case 73: boyshp = 7; break;
            case 85: boyshp = 8; break;
            default: break;
        }
        poFeatureO->SetField(poFeatureO->GetFieldIndex("BOYSHP"), boyshp);
    }
    if (feature->GetFieldIndex("btc") != -1 && poFeatureO->GetFieldIndex("CATSPM") != -1) {
        int ssc = feature->GetFieldAsInteger(feature->GetFieldIndex("btc"));
        int boyshp = 0;
        switch (ssc) 
        {
            case 4: boyshp = 15; break;
            case 7: boyshp = 14; break;
            case 10: boyshp = 9; break;
            default: break;
        }
        poFeatureO->SetField(poFeatureO->GetFieldIndex("CATSPM"), boyshp);
    }
   
    //poFeatureO->DumpReadable(0,0);
    encLayer = _outputSource->GetLayer(poFeatureO->GetFieldAsInteger("OBJL")+3);
    if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in shapefile.\n" );   
    }


    return;
}


void FeatureHandler::WriteFeatureRecords(vector<GeoRef*> geoRefs, int numSegments) {

    int layers = _dataSource->GetLayerCount();
    OGRLayer *layer;
    BuildTables(&_lookuptable);
    _globalSegmentID = numSegments;
    _refList = geoRefs;
    //reset the featureID back to 0, so the number matches when we re-read the file
    int featureID = 0;

    //sort the georefs so we can look them up (from the FeatureID)
    std::sort(_refList.begin(), _refList.end(), GeoRef::IDSortPredicate);

    for (int l = 0 ; l< layers; l++) {
        layer = _dataSource->GetLayer(l);
        if (layer == NULL || layer->GetFeatureCount() == 0) continue;
        
        OGRFeature *feature;
        layer->ResetReading();
        while( (feature = layer->GetNextFeature()) != NULL ) {
            featureID++;
            ReProcessFeature(feature, featureID);
        }
    }
}
