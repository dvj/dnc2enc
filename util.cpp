#include "ogrsf_frmts.h"
//#include "s57.h"
int main(int argc, char **argv)

{
    OGRRegisterAll();

    OGRDataSource       *poDS;
    printf("Opening %s\n",argv[1]);
    poDS = OGRSFDriverRegistrar::Open( argv[1], FALSE );
    if( poDS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }

    OGRLayer  *poLayer;
    int layers = poDS->GetLayerCount();
    for (int layer =0 ; layer< layers; layer++) {
        poLayer = poDS->GetLayer(layer);
        if (poLayer == NULL) continue;
        printf("%d, %s, %s",layer, poLayer->GetName(), OGRGeometryTypeToName(poLayer->GetGeomType()));
        poLayer->ResetReading();
        OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();  
        int iField;
        for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
        {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
            printf(", %s",poFieldDefn->GetNameRef());
        }
        printf("\n");
    }

}
