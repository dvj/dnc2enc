#include "mys57.h"
#include "ogrsf_frmts.h"
#include "buildtables.h"

using namespace std;


int gID = 10;
int ENCLayerNum = 0;
   
void ShowLayerDebugInfo(OGRLayer *layer) {
    printf("Reading Layer %s of type %s\n",layer->GetName(), OGRGeometryTypeToName(layer->GetGeomType()));
    OGRFeatureDefn *poFDefn = layer->GetLayerDefn();
    printf("Layer Def: %s\n",poFDefn->GetName());    
    
    int iField;
    for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
    {
        OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
        printf("LF:  %s\n",poFieldDefn->GetNameRef());
    }
}

int HandleGeometry(OGRFeature *poFeature, OGRDataSource *poOUT) {
    int madeGeoType = 0;
    OGRGeometry *poGeometry;
    poGeometry = poFeature->GetGeometryRef();
    if( poGeometry == NULL) {
        return 0;
    }
    // double x,y;
    OGRFeature *poFeatureO = NULL;
    OGRLayer *encLayer;
    //OGRFeatureDefn * fd = new OGRFeatureDefn("IsolatedNode");

    if (wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
        printf("Making POINT\n");
        encLayer = poOUT->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);        
        OGRPoint *poPoint = (OGRPoint *) poGeometry;
        OGRPoint pt;
        pt.setX( poPoint->getX() );
        pt.setY( poPoint->getY() );
        poFeatureO->SetGeometry( poGeometry );
        madeGeoType = RCNM_VI;
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbLineString ) {
        printf("Making LINESTRING\n");
        encLayer = poOUT->GetLayer(2);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());                
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VE);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_0"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_0"),0); //FIXME: fill this in
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_0"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_0"),255);
        madeGeoType = RCNM_VE;
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) {
        printf("Making POLYGON\n");
        encLayer = poOUT->GetLayer(2);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        //poFeatureO->SetField(poFeatureO->GetFieldIndex("PRIM"),3);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VE);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM_0"),RCNM_VC);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID_0"),0); //FIXME: fill this in
        poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG_0"),255);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("TOPI_0"),1);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK_0"),255);
        OGRPolygon *poly = (OGRPolygon *) poGeometry;
        OGRLineString *ls = (OGRLineString *) poly->getExteriorRing();
        
        poGeometry = ls;
        madeGeoType = RCNM_VE;   
    } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPoint) {
        printf("Making MULTIPOINT\n");
        encLayer = poOUT->GetLayer(0);
        poFeatureO->SetField(poFeatureO->GetFieldIndex("RCNM"),RCNM_VI);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        //poFeatureO->SetField(poFeatureO->GetFieldIndex("PRIM"),1);
        madeGeoType = RCNM_VI;
    } else {
        printf("OTHER GEOMETRY OF TYPE %d\n",poGeometry->getGeometryType());
        encLayer = poOUT->GetLayer(0);
        poFeatureO = OGRFeature::CreateFeature(encLayer->GetLayerDefn());            
        //poFeatureO->SetField(poFeatureO->GetFieldIndex("PRIM"),4);
        madeGeoType = RCNM_VI;
    }
    poFeatureO->SetGeometry( poGeometry );
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("OBJL"),ENCLayerNum);
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("GRUP"),0); //FIXME - this should be 0 or 2
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("RVER"),1);
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RUIN"),1);
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("AGEN"),40); //FIXME - find a good agency
    //poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),poFeature->GetFieldAsInteger(poFeature->GetFieldIndex("id")));
    poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),gID++);
    if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in shapefile.\n" );
        return 0;
    }
    // poFeatureO->DumpReadable(0,0);
    OGRFeature::DestroyFeature(poFeatureO);
    return madeGeoType;
}


int main(int argc, char **argv)

{
    map<string,int> lookuptable;
    BuildTables(&lookuptable);
    // const char *pszDriverName = "ESRI Shapefile";
     const char *pszDriverName = "S57";
    OGRSFDriver *poDriver;

    OGRRegisterAll();

    OGRDataSource       *poDS;
    printf("Opening %s\n",argv[1]);
    poDS = OGRSFDriverRegistrar::Open( argv[1], FALSE );
    if( poDS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }
    
    //now setup the writter
    poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
                pszDriverName );
    if( poDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }
    OGRDataSource *poOUT = poDriver->CreateDataSource( "testENC.000", NULL );
    if( poDS == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }    
    OGRLayer  *poLayer;
    OGRLayer *encLayer = NULL;
    OGRFeature **poFeatureList = new OGRFeature*[10000];
    int f =0;
    int layers = poDS->GetLayerCount();
    layers = 50; //FIXME - remove this (stops random segfault)
    for (int layer = 0 ; layer< layers; layer++) {
        
        poLayer = poDS->GetLayer(layer);
        if (poLayer == NULL || poLayer->GetFeatureCount() == 0) continue;
                
        OGRFeature *poFeature;
        poLayer->ResetReading();
        //poLayerO = poOUT->GetLayerByName(poLayer->GetName());
        
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry == NULL) {
                //TODO - do something with the non-geometry
                continue;
            }

            int geometryType = 0;
            const char *f_code;
            char *sLookup = new char[64];
            
            if (poFeature->GetFieldIndex("f_code") != -1) {
                f_code = poFeature->GetFieldAsString(poFeature->GetFieldIndex("f_code"));
                strcpy(sLookup,f_code);
            }
            if (wkbFlatten(poGeometry->getGeometryType()) == wkbPoint || 
                wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPoint) {
                geometryType = 1;
                strcat(sLookup,"_Point");
            } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbLineString ) {
                geometryType = 2;
                strcat(sLookup,"_Line");
            } else if (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon ) {
                geometryType = 3;
                strcat(sLookup,"_Area");
            }
             
            ENCLayerNum = lookuptable[sLookup];
            if (ENCLayerNum == 0  || ENCLayerNum == -1) {
                printf("No available layer for %s, %s\n",poLayer->GetName(), sLookup);
                continue;
            }
            if (gID == 530) {
                printf("RECORD 530\n");
                poFeature->DumpReadable(0,0);
            }
            encLayer = poOUT->GetLayer(ENCLayerNum+3);
            //printf("Using ENCLayer %s for DNC Layer %s\n",encLayer->GetName(), poLayer->GetName());
            //poFeature->DumpReadable(0,0);

            int madeGeo = 0;
            madeGeo = HandleGeometry(poFeature, poOUT);

            OGRFeature *poFeatureO;
            
            //poFeatureList[f] = OGRFeature::CreateFeature( encLayer->GetLayerDefn() );
            poFeatureList[f] = new OGRFeature(encLayer->GetLayerDefn());
            poFeatureO = poFeatureList[f];
            poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),10000+gID++);
            
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
            
            poFeatureO->SetField(poFeatureO->GetFieldIndex("FIDN"),poFeature->GetFieldAsInteger(poFeature->GetFieldIndex("id")));
            if (madeGeo != 0) {
                int *gl = new int[1];
                gl[0] = madeGeo;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM"),1,gl);
                gl[0] = gID-2;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID"),1,gl);
                gl[0] = 1;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT"),1,gl);
                gl[0] = 1;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG"),1,gl);
                gl[0] = 2;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK"),2,gl);
                
                //poFeatureO->SetField(poFeatureO->GetFieldIndex("LNAM_REFS"),
            }
            /*
              for(int iField = 0; iField < poFeature->GetFieldCount(); iField++ )
            {
                OGRFieldDefn *poFieldDefn = poFeature->GetFieldDefnRef( iField );
                if (!poFeature->IsFieldSet(iField)) continue;
                //printf("  %s %s\n",poFieldDefn->GetNameRef(),poFeature->GetFieldAsString(iField) );
                if( poFieldDefn->GetType() == OFTInteger )
                    poFeatureO->SetField(iField,(poFeature->GetFieldAsInteger(iField)));
                else if( poFieldDefn->GetType() == OFTReal )
                    poFeatureO->SetField(iField,(poFeature->GetFieldAsDouble(iField)));
                else if( poFieldDefn->GetType() == OFTString )
                    poFeatureO->SetField(iField,(poFeature->GetFieldAsString(iField)));
                else if (poFieldDefn->GetType() == OFTIntegerList ) {
                    poFeature->GetFieldAsIntegerList(iField, &cnt);
                    int *list = new int[cnt];
                    const int *v = poFeature->GetFieldAsIntegerList(iField, &cnt);
                    for (int j=0;j< cnt;j++) {
                        list[j] = v[j];
                    }
                    poFeatureO->SetField(iField,cnt, list);
                }
                else if (poFieldDefn->GetType() == OFTRealList ){
                    poFeature->GetFieldAsDoubleList(iField, &cnt);
                    double *list = new double[cnt];
                    const double *v = poFeature->GetFieldAsDoubleList(iField, &cnt);
                    for (int j=0;j< cnt;j++) {
                        list[j] = v[j];
                    }
                    poFeatureO->SetField(iField,cnt, list);
                }
            }*/
            
           

       
            // poFeature->DumpReadable(0,0);
            // printf("----------------------------------\n");
           
            
            // poFeatureList[f++] = poFeatureO;
            f++;
            

            //printf("Writing out feature\n");
            
           
        }
       
    }
    printf("gID: %d\n",gID);
    OGRFeature *poFeatureO;
        
        for (int i=0;i<f;i++) {
            poFeatureO = poFeatureList[i];
            //poFeatureO->DumpReadable(0,0);
            encLayer = poOUT->GetLayer(poFeatureO->GetFieldAsInteger("OBJL")+3);
            if( encLayer->CreateFeature( poFeatureO ) != OGRERR_NONE )
            {
                printf( "Failed to create feature in shapefile.\n" );   
            }
            //OGRFeature::DestroyFeature( poFeatureO );
        }
    printf("Done, exiting \n");
    OGRDataSource::DestroyDataSource( poOUT );
    OGRDataSource::DestroyDataSource( poDS );
    
}
