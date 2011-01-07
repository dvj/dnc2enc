#include "geohandler.h"
#include "mys57.h"

GeoHandler::GeoHandler(OGRDataSource *dat) {
    _gID = 0;
    _dataSource = dat;

}


GeoHandler::~GeoHandler() {


}


int GeoHandler::ReadGeometry() {


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
