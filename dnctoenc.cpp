#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "ogrsf_frmts.h"
#include "buildtables.h"
#include "geohandler.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

using namespace std;


int fID = 20000;
int ENCLayerNum = 0;
   
void ShowLayerDebugInfo(OGRLayer *layer) {
    printf("Reading Layer %s of type %s\n",layer->GetLayerDefn()->GetName(), OGRGeometryTypeToName(layer->GetLayerDefn()->GetGeomType()));
    OGRFeatureDefn *poFDefn = layer->GetLayerDefn();
    printf("Layer Def: %s\n",poFDefn->GetName());    
    
    int iField;
    for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
    {
        OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
        printf("LF:  %s\n",poFieldDefn->GetNameRef());
    }
}



OGRDataSource* openInputFile(char *input) {
    OGRRegisterAll();
    OGRDataSource  *poDS;
    char *input2 = new char[PATH_MAX+strlen(input)+10];
    strcpy(input2, "gltp:/vrf");
    realpath(input, input2+9);
    printf("Opening %s\n",input2);
    poDS = OGRSFDriverRegistrar::Open( input2, FALSE );
    if( poDS == NULL )
    {
        printf( "Open failed. Did you give the full path of a DNC directory as the first argument?\n" );
        exit( 1 );
    }
    delete[] input2;
    return poDS;
}

OGRDataSource *openOutputFile(char *output) {
    const char *pszDriverName = "S57";
    OGRSFDriver *poDriver;
    poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(pszDriverName );
    if( poDriver == NULL ) {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }
    OGRDataSource *poOUT = poDriver->CreateDataSource( output, NULL );
    if( poOUT == NULL ) {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }
    return poOUT;
}

int ProcessFeatures(OGRDataSource *poDS, OGRDataSource *poOUT) {
    
    OGRLayer *poLayer;
    OGRLayer *encLayer = NULL;
    OGRFeature **poFeatureList = new OGRFeature*[20000];

    int f = 0;    
    map<string,int> lookuptable;
    BuildTables(&lookuptable);
    GeoHandler *geoHandler = new GeoHandler(poDS,poOUT);
        
    int layers = poDS->GetLayerCount();
    // layers = 50; //FIXME - remove this (stops random segfault)
    for (int layer = 0 ; layer< layers; layer++) {        
        poLayer = poDS->GetLayer(layer);
        if (poLayer == NULL || poLayer->GetFeatureCount() == 0) continue;
                
        OGRFeature *poFeature;
        poLayer->ResetReading();
        
        while( (poFeature = poLayer->GetNextFeature()) != NULL )
        {
            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry == NULL) {
                printf("Non-geometry on layer %s\n", poLayer->GetLayerDefn()->GetName());
                //TODO - do something with the non-geometry
                continue;
            }

            int geometryType = 0;
            const char *f_code;
            char *sLookup = new char[64];
            
            if (poFeature->GetFieldIndex("f_code") != -1) {
                f_code = poFeature->GetFieldAsString(poFeature->GetFieldIndex("f_code"));
                strcpy(sLookup,f_code);
            } else {
                //printf("No f_code\n");
                strcpy(sLookup,"");
                //TODO - Do something useful with these layers
                /*
                for(int iField = 0; iField < poFeature->GetFieldCount(); iField++ )
                {
                    OGRFieldDefn *poFieldDefn = poFeature->GetFieldDefnRef( iField );
                    if (!poFeature->IsFieldSet(iField)) continue;
                    printf("  %s: %s\n",poFieldDefn->GetNameRef(),poFeature->GetFieldAsString(iField));
                }
                */
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
                //printf("No available layer for %s, %s\n",poLayer->GetLayerDefn()->GetName(), sLookup);
                continue;
            }
            
            encLayer = poOUT->GetLayer(ENCLayerNum+3);
            //printf("Using ENCLayer %s for DNC Layer %s\n",encLayer->GetName(), poLayer->GetName());
            //poFeature->DumpReadable(0,0);

            int madeGeo = 0;
            madeGeo = geoHandler->HandleGeometry(poFeature);

            OGRFeature *poFeatureO;
            
            //poFeatureList[f] = OGRFeature::CreateFeature( encLayer->GetLayerDefn() );
            poFeatureList[f] = new OGRFeature(encLayer->GetLayerDefn());
            poFeatureO = poFeatureList[f];
            poFeatureO->SetField(poFeatureO->GetFieldIndex("RCID"),fID++);
            
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
            if (madeGeo > 0) {
                int *gl = new int[1];
                gl[0] = madeGeo;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCNM"),1,gl);
                gl[0] = geoHandler->GetGID()-1;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("NAME_RCID"),1,gl);
                gl[0] = 1;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("ORNT"),1,gl);
                gl[0] = 1;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("USAG"),1,gl);
                gl[0] = 2;
                poFeatureO->SetField(poFeatureO->GetFieldIndex("MASK"),1,gl);
                
                //poFeatureO->SetField(poFeatureO->GetFieldIndex("LNAM_REFS"),
            }
            if (poFeature->GetFieldIndex("nam") != -1 && poFeatureO->GetFieldIndex("OBJNAM") != -1) {
                poFeatureO->SetField(poFeatureO->GetFieldIndex("OBJNAM"), poFeature->GetFieldAsString(poFeature->GetFieldIndex("nam")));
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
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <DNC directory> <output file>\n",argv[0]);
        exit(0);
    }
    OGRDataSource  *poDS = openInputFile(argv[1]);
    OGRDataSource *poOUT = openOutputFile(argv[2]);

    
    GeoHandler *geoHandler = new GeoHandler(poDS, poOUT);
    
    geoHandler->ReadGeometry();

    //ProcessFeatures(poDS, poOUT);

    OGRDataSource::DestroyDataSource( poOUT );
    OGRDataSource::DestroyDataSource( poDS );
    
}
