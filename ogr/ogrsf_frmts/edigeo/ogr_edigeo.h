/******************************************************************************
 *
 * Project:  EDIGEO Translator
 * Purpose:  Definition of classes for OGR .edigeo driver.
 * Author:   Even Rouault, even dot rouault at spatialys.com
 *
 ******************************************************************************
 * Copyright (c) 2011, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef OGR_EDIGEO_H_INCLUDED
#define OGR_EDIGEO_H_INCLUDED

#include "ogrsf_frmts.h"
#include <vector>
#include <map>
#include <set>

/************************************************************************/
/*                           OGREDIGEOLayer                             */
/************************************************************************/

class OGREDIGEODataSource;

class OGREDIGEOLayer final : public OGRLayer,
                             public OGRGetNextFeatureThroughRaw<OGREDIGEOLayer>
{
    OGREDIGEODataSource *poDS;

    OGRFeatureDefn *poFeatureDefn;
    OGRSpatialReference *poSRS;

    int nNextFID;

    OGRFeature *GetNextRawFeature();

    std::vector<OGRFeature *> aosFeatures;

    /* Map attribute RID ('TEX2_id') to its index in the OGRFeatureDefn */
    std::map<CPLString, int> mapAttributeToIndex;

  public:
    OGREDIGEOLayer(OGREDIGEODataSource *poDS, const char *pszName,
                   OGRwkbGeometryType eType, OGRSpatialReference *poSRS);
    virtual ~OGREDIGEOLayer();

    virtual void ResetReading() override;
    DEFINE_GET_NEXT_FEATURE_THROUGH_RAW(OGREDIGEOLayer)
    virtual OGRFeature *GetFeature(GIntBig nFID) override;
    virtual GIntBig GetFeatureCount(int bForce) override;

    virtual OGRFeatureDefn *GetLayerDefn() override
    {
        return poFeatureDefn;
    }

    virtual int TestCapability(const char *) override;

    void AddFeature(OGRFeature *poFeature);

    int GetAttributeIndex(const CPLString &osRID);
    void AddFieldDefn(const CPLString &osName, OGRFieldType eType,
                      const CPLString &osRID);
};

/************************************************************************/
/*                         OGREDIGEODataSource                          */
/************************************************************************/

typedef std::pair<int, int> intintType;
typedef std::pair<double, double> xyPairType;
typedef std::vector<xyPairType> xyPairListType;
typedef std::pair<CPLString, CPLString> strstrType;
typedef std::vector<CPLString> strListType;

/* From the .DIC file */
class OGREDIGEOAttributeDef
{
  public:
    OGREDIGEOAttributeDef()
    {
    }

    CPLString osLAB; /* e.g. TEX2 */
    CPLString osTYP; /* e.g. T */
};

/* From the .SCD file */
class OGREDIGEOObjectDescriptor
{
  public:
    OGREDIGEOObjectDescriptor()
    {
    }

    CPLString osRID;        /* e.g. BATIMENT_id */
    CPLString osNameRID;    /* e.g. ID_N_OBJ_E_2_1_0 */
    CPLString osKND;        /* e.g. ARE */
    strListType aosAttrRID; /* e.g. DUR_id, TEX_id */
};

/* From the .SCD file */
class OGREDIGEOAttributeDescriptor
{
  public:
    OGREDIGEOAttributeDescriptor() : nWidth(0)
    {
    }

    CPLString osRID;     /* e.g. TEX2_id */
    CPLString osNameRID; /* e.g. ID_N_ATT_TEX2 */
    int nWidth;          /* e.g. 80 */
};

/* From the .VEC files */
class OGREDIGEOFEADesc
{
  public:
    OGREDIGEOFEADesc()
    {
    }

    std::vector<strstrType>
        aosAttIdVal;     /* e.g. (TEX2_id,BECHEREL),(IDU_id,022) */
    CPLString osSCP;     /* e.g. COMMUNE_id */
    CPLString osQUP_RID; /* e.g. Actualite_Objet_X */
};

class OGREDIGEODataSource final : public GDALDataset
{
    friend class OGREDIGEOLayer;

    VSILFILE *fpTHF;

    OGRLayer **papoLayers;
    int nLayers;

    VSILFILE *OpenFile(const char *pszType, const CPLString &osExt);

    // TODO: Translate comments to English.
    CPLString osLON; /* Nom du lot */
    CPLString osGNN; /* Nom du sous-ensemble de données générales */
    CPLString osGON; /* Nom du sous-ensemble de la référence de coordonnées */
    CPLString osQAN; /* Nom du sous-ensemble de qualité */
    CPLString osDIN; /* Nom du sous-ensemble de définition de la nomenclature */
    CPLString osSCN;    /* Nom du sous-ensemble de définition du SCD */
    strListType aosGDN; /* Nom du sous-ensemble de données géographiques */
    int ReadTHF(VSILFILE *fp);

    CPLString osREL;
    OGRSpatialReference *poSRS;
    int ReadGEO();

    /* Map from ID_N_OBJ_E_2_1_0 to OBJ_E_2_1_0 */
    std::map<CPLString, CPLString> mapObjects;

    /* Map from ID_N_ATT_TEX2 to (osLAB=TEX2, osTYP=T) */
    std::map<CPLString, OGREDIGEOAttributeDef> mapAttributes;
    int ReadDIC();

    std::vector<OGREDIGEOObjectDescriptor> aoObjList;
    /* Map from TEX2_id to (osNameRID=ID_N_ATT_TEX2, nWidth=80) */
    std::map<CPLString, OGREDIGEOAttributeDescriptor> mapAttributesSCD;
    int ReadSCD();

    int bExtentValid;
    double dfMinX;
    double dfMinY;
    double dfMaxX;
    double dfMaxY;
    int ReadGEN();

    /* Map from Actualite_Objet_X to (creationData, updateData) */
    std::map<CPLString, intintType> mapQAL;
    int ReadQAL();

    std::map<CPLString, OGREDIGEOLayer *> mapLayer;

    int CreateLayerFromObjectDesc(const OGREDIGEOObjectDescriptor &objDesc);

    std::map<CPLString, xyPairType> mapPNO; /* Map Noeud_X to (x,y) */
    std::map<CPLString, xyPairListType>
        mapPAR; /* Map Arc_X to ((x1,y1),...(xn,yn)) */
    std::map<CPLString, OGREDIGEOFEADesc> mapFEA; /* Map Object_X to FEADesc */
    std::map<CPLString, strListType>
        mapPFE_PAR; /* Map Face_X to (Arc_X1,..Arc_Xn) */
    std::vector<std::pair<CPLString, strListType>>
        listFEA_PFE; /* List of (Object_X,(Face_Y1,..Face_Yn)) */
    std::vector<std::pair<CPLString, strListType>>
        listFEA_PAR; /* List of (Object_X,(Arc_Y1,..Arc_Yn))) */
    std::vector<strstrType> listFEA_PNO; /* List of (Object_X,Noeud_Y) */
    std::map<CPLString, CPLString>
        mapFEA_FEA; /* Map Attribut_TEX{X}_id_Objet_{Y} to Objet_Y */

    int bRecodeToUTF8;
    int bHasUTF8ContentOnly;

    int ReadVEC(const char *pszVECName);

    OGRFeature *CreateFeature(const CPLString &osFEA);
    int BuildPoints();
    int BuildLineStrings();
    int BuildPolygon(const CPLString &osFEA, const strListType &aosPFE);
    int BuildPolygons();

    int iATR, iDI3, iDI4, iHEI, iFON;
    int iATR_VAL, iANGLE, iSIZE, iOBJ_LNK, iOBJ_LNK_LAYER;
    double dfSizeFactor;
    int bIncludeFontFamily;
    int SetStyle(const CPLString &osFEA, OGRFeature *poFeature);

    std::set<CPLString> setLayersWithLabels;
    void CreateLabelLayers();

    int bHasReadEDIGEO;
    void ReadEDIGEO();

  public:
    OGREDIGEODataSource();
    virtual ~OGREDIGEODataSource();

    int Open(const char *pszFilename);

    virtual int GetLayerCount() override;
    virtual OGRLayer *GetLayer(int) override;

    int HasUTF8ContentOnly()
    {
        return bHasUTF8ContentOnly;
    }
};

#endif /* ndef OGR_EDIGEO_H_INCLUDED */
