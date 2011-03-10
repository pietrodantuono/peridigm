/*
 * Peridigm_IsotropicElasticPlasticMaterial.cxx
 *
 */
#include "Peridigm_IsotropicElasticPlasticMaterial.hpp"
#include "Peridigm_CriticalStretchDamageModel.hpp"
#include <Teuchos_TestForException.hpp>
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "PdMaterialUtilities.h"
#include <limits>


PeridigmNS::IsotropicElasticPlasticMaterial::IsotropicElasticPlasticMaterial(const Teuchos::ParameterList & params)
:
Material(params),
m_damageModel()
{
	//! \todo Add meaningful asserts on material properties.
	m_bulkModulus = params.get<double>("Bulk Modulus");
	m_shearModulus = params.get<double>("Shear Modulus");
	m_horizon = params.get<double>("Material Horizon");
	m_density = params.get<double>("Density");
	m_yieldStress = params.get<double>("Yield Stress");

	/*
	 * Set the yield stress to a very large value: this in effect makes the model run elastic -- useful for testing
	 */
	if(params.isType<string>("Test"))
		m_yieldStress = std::numeric_limits<double>::max( );

	if(params.isSublist("Damage Model")){
		Teuchos::ParameterList damageParams = params.sublist("Damage Model");
		if(!damageParams.isParameter("Type")){
			TEST_FOR_EXCEPTION(true, Teuchos::Exceptions::InvalidParameter,
					"Damage model \"Type\" not specified in Damage Model parameter list.");
		}
		string& damageModelType = damageParams.get<string>("Type");
		if(damageModelType == "Critical Stretch"){
			m_damageModel = Teuchos::rcp(new PeridigmNS::CriticalStretchDamageModel(damageParams));
		}
		else{
			TEST_FOR_EXCEPTION(true, Teuchos::Exceptions::InvalidParameter,
					"Invalid damage model, \"None\" or \"Critical Stretch\" required.");
		}
	}

    // set up vector of variable specs
    m_variableSpecs = Teuchos::rcp(new std::vector<Field_NS::FieldSpec>);
    m_variableSpecs->push_back(Field_NS::VOLUME);
    m_variableSpecs->push_back(Field_NS::DAMAGE);
    m_variableSpecs->push_back(Field_NS::WEIGHTED_VOLUME);
    m_variableSpecs->push_back(Field_NS::DILATATION);
    m_variableSpecs->push_back(Field_NS::COORD3D);
    m_variableSpecs->push_back(Field_NS::CURCOORD3D);
    m_variableSpecs->push_back(Field_NS::FORCE_DENSITY3D);
    m_variableSpecs->push_back(Field_NS::DEVIATORIC_PLASTIC_EXTENSION);
    m_variableSpecs->push_back(Field_NS::LAMBDA);
    m_variableSpecs->push_back(Field_NS::BOND_DAMAGE);
    m_variableSpecs->push_back(Field_NS::SHEAR_CORRECTION_FACTOR);

}


PeridigmNS::IsotropicElasticPlasticMaterial::~IsotropicElasticPlasticMaterial()
{
}


void PeridigmNS::IsotropicElasticPlasticMaterial::initialize(const double dt,
                                                             const int numOwnedPoints,
                                                             const int* ownedIDs,
                                                             const int* neighborhoodList,
                                                             PeridigmNS::DataManager& dataManager) const
{
	  // Extract pointers to the underlying data
      double *xOverlap, *cellVolumeOverlap, *weightedVolume, *shear_correction_factor;
      dataManager.getData(Field_NS::COORD3D, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&xOverlap);
      dataManager.getData(Field_NS::VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&cellVolumeOverlap);
      dataManager.getData(Field_NS::WEIGHTED_VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&weightedVolume);
      dataManager.getData(Field_NS::SHEAR_CORRECTION_FACTOR, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&shear_correction_factor);

	  PdMaterialUtilities::computeWeightedVolume(xOverlap,cellVolumeOverlap,weightedVolume,numOwnedPoints,neighborhoodList);
//	  PdMaterialUtilities::computeShearCorrectionFactor(numOwnedPoints,xOverlap,cellVolumeOverlap,neighborhoodList,m_horizon,shear_correction_factor);
}

void
PeridigmNS::IsotropicElasticPlasticMaterial::updateConstitutiveData(const double dt,
                                                                    const int numOwnedPoints,
                                                                    const int* ownedIDs,
                                                                    const int* neighborhoodList,
                                                                    PeridigmNS::DataManager& dataManager) const
{
  // Extract pointers to the underlying data in the constitutiveData array
  double *x, *y, *volume, *dilatation, *damage, *weightedVolume, *bondDamage;
  dataManager.getData(Field_NS::COORD3D, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&x);
  dataManager.getData(Field_NS::CURCOORD3D, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&y);
  dataManager.getData(Field_NS::VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&volume);
  dataManager.getData(Field_NS::DILATATION, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&dilatation);
  dataManager.getData(Field_NS::DAMAGE, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&damage);
  dataManager.getData(Field_NS::WEIGHTED_VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&weightedVolume);
  dataManager.getData(Field_NS::BOND_DAMAGE, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&bondDamage);

	// Update the bond damage
	if(!m_damageModel.is_null()){
		m_damageModel->computeDamage(dt,
                                     numOwnedPoints,
                                     ownedIDs,
                                     neighborhoodList,
                                     dataManager);
	}

	//  Update the element damage (percent of bonds broken)
	int neighborhoodListIndex = 0;
	int bondIndex = 0;
	for(int iID=0 ; iID<numOwnedPoints ; ++iID){
		int nodeID = ownedIDs[iID];
		int numNeighbors = neighborhoodList[neighborhoodListIndex++];
		neighborhoodListIndex += numNeighbors;
		double totalDamage = 0.0;
		for(int iNID=0 ; iNID<numNeighbors ; ++iNID){
			totalDamage += bondDamage[bondIndex++];
		}
		if(numNeighbors > 0)
			totalDamage /= numNeighbors;
		else
			totalDamage = 0.0;
		damage[nodeID] = totalDamage;
	}

	PdMaterialUtilities::computeDilatation(x,y,weightedVolume,volume,bondDamage,dilatation,neighborhoodList,numOwnedPoints);
}

void
PeridigmNS::IsotropicElasticPlasticMaterial::computeForce(const double dt,
                                                          const int numOwnedPoints,
                                                          const int* ownedIDs,
                                                          const int* neighborhoodList,
                                                          PeridigmNS::DataManager& dataManager) const
{
 
	  // Extract pointers to the underlying data in the constitutiveData array
      double *x, *y, *volume, *dilatation, *weightedVolume, *bondDamage, *edpN, *edpNP1, *lambdaN, *lambdaNP1, *force;
      dataManager.getData(Field_NS::COORD3D, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&x);
      dataManager.getData(Field_NS::CURCOORD3D, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&y);
      dataManager.getData(Field_NS::VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&volume);
      dataManager.getData(Field_NS::DILATATION, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&dilatation);
      dataManager.getData(Field_NS::WEIGHTED_VOLUME, Field_NS::FieldSpec::STEP_NONE)->ExtractView(&weightedVolume);
      dataManager.getData(Field_NS::BOND_DAMAGE, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&bondDamage);
      dataManager.getData(Field_NS::DEVIATORIC_PLASTIC_EXTENSION, Field_NS::FieldSpec::STEP_N)->ExtractView(&edpN);
      dataManager.getData(Field_NS::DEVIATORIC_PLASTIC_EXTENSION, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&edpNP1);
      dataManager.getData(Field_NS::LAMBDA, Field_NS::FieldSpec::STEP_N)->ExtractView(&lambdaN);
      dataManager.getData(Field_NS::LAMBDA, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&lambdaNP1);
      dataManager.getData(Field_NS::FORCE_DENSITY3D, Field_NS::FieldSpec::STEP_NP1)->ExtractView(&force);

      // Zero out the force
      dataManager.getData(Field_NS::FORCE_DENSITY3D, Field_NS::FieldSpec::STEP_NP1)->PutScalar(0.0);

	  PdMaterialUtilities::computeInternalForceIsotropicElasticPlastic
        (x,
         y,
         weightedVolume,
         volume,
         dilatation,
         bondDamage,
         edpN,
         edpNP1,
         lambdaN,
         lambdaNP1,
         force,
         neighborhoodList,
         numOwnedPoints,
         m_bulkModulus,
         m_shearModulus,
         m_horizon,
         m_yieldStress);

}

