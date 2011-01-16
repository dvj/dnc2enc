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
