/*! \file PdMaterialUtilities.cxx */

//@HEADER
// ************************************************************************
//
//                             Peridigm
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?
// David J. Littlewood   djlittl@sandia.gov
// John A. Mitchell      jamitch@sandia.gov
// Michael L. Parks      mlparks@sandia.gov
// Stewart A. Silling    sasilli@sandia.gov
//
// ************************************************************************
//@HEADER

#include "PdMaterialUtilities.h"
#include <math.h>
#include <iostream>
#include <string>

namespace PdMaterialUtilities {


const std::string NAMESPACE="PdMaterialUtilities::";

void updateGeometry
(
		const double* xOverlap,
		const double* uOverlap,
		const double* velocityOverlap,
		double* yOverlap,
		int overLapLength,
		double dt
)
{
	const double* x = xOverlap;
	const double* u = uOverlap;
	const double* v = velocityOverlap;
	double*       y = yOverlap;

	int length = overLapLength;
	for(;x!=xOverlap+length;x++,u++,v++,y++)
		*y = *x + *u + *v * dt;
}


void computeWeightedVolume
(
		const double* xOverlap,
		const double* volumeOverlap,
		double *mOwned,
		int myNumPoints,
		const int* localNeighborList
){
	double *m = mOwned;
	const double *xOwned = xOverlap;
	const int *neighPtr = localNeighborList;
	for(int p=0;p<myNumPoints;p++, xOwned+=3, m++){
		int numNeigh = *neighPtr;
		const double *X = xOwned;
		*m=computeWeightedVolume(X,xOverlap,volumeOverlap,neighPtr);
		neighPtr+=(numNeigh+1);
	}
}


void computeDilatation
(
		const double* xOverlap,
		const double* yOverlap,
		const double *mOwned,
		const double* volumeOverlap,
		const double* bondDamage,
		double* dilatationOwned,
		const int* localNeighborList,
		int numOwnedPoints
)
{
	double OMEGA=1.0;
	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	double *theta = dilatationOwned;
	double cellVolume;
	const int *neighPtr = localNeighborList;
	for(int p=0; p<numOwnedPoints;p++, xOwned+=3, yOwned+=3, m++, theta++){
		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		*theta = 0;
		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			double dx = XP[0]-X[0];
			double dy = XP[1]-X[1];
			double dz = XP[2]-X[2];
			double zetaSqared = dx*dx+dy*dy+dz*dz;
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			double dY = dx*dx+dy*dy+dz*dz;
			double d = sqrt(zetaSqared);
			double e = sqrt(dY)-d;
			*theta += 3.0*OMEGA*(1.0-*bondDamage)*d*e*cellVolume/(*m);
		}

	}
}

//! Version of computeDilataion for arbitrary list of ownedIDs
void computeDilatation
(
		const double* xOverlap,
		const double* yOverlap,
		const double *mOwned,
		const double* volumeOverlap,
		const double* bondDamage,
		double* dilatationOwned,
        const int* ownedIDs,
		const int* localNeighborList,
		int numOwnedPoints
)
{
	double OMEGA=1.0;
	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	double *theta = dilatationOwned;
	double cellVolume;
	const int *neighPtr = localNeighborList;
	for(int p=0; p<numOwnedPoints;p++){

        int ID = ownedIDs[p];
        xOwned = &xOverlap[3*ID];
        yOwned = &yOverlap[3*ID];
        m = &mOwned[ID];
        theta = &dilatationOwned[ID];

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		*theta = 0;
		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			double dx = XP[0]-X[0];
			double dy = XP[1]-X[1];
			double dz = XP[2]-X[2];
			double zetaSqared = dx*dx+dy*dy+dz*dz;
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			double dY = dx*dx+dy*dy+dz*dz;
			double d = sqrt(zetaSqared);
			double e = sqrt(dY)-d;
			*theta += 3.0*OMEGA*(1.0-*bondDamage)*d*e*cellVolume/(*m);
		}

	}
}


void computeInternalForceLinearElastic
(
		const double* xOverlap,
		const double* yOverlap,
		const double* mOwned,
		const double* volumeOverlap,
		const double* dilatationOwned,
		const double* bondDamage,
		double* fInternalOverlap,
		const int*  localNeighborList,
		int numOwnedPoints,
		double BULK_MODULUS,
		double SHEAR_MODULUS
)
{

	/*
	 * Compute processor local contribution to internal force
	 */
	double K = BULK_MODULUS;
	double MU = SHEAR_MODULUS;
	double OMEGA=1.0;

	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	const double *theta = dilatationOwned;
	double *fOwned = fInternalOverlap;

	const int *neighPtr = localNeighborList;
	double cellVolume, alpha, dx, dy, dz, zeta, dY, t;
	for(int p=0;p<numOwnedPoints;p++, xOwned +=3, yOwned +=3, fOwned+=3, m++, theta++){

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		alpha = 15.0*MU/(*m);
		double selfCellVolume = v[p];
		double c1 = OMEGA*(*theta)*(9.0*K-15.0*MU)/(3.0*(*m));
		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			dx = XP[0]-X[0];
			dy = XP[1]-X[1];
			dz = XP[2]-X[2];
			zeta = sqrt(dx*dx+dy*dy+dz*dz);
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			dY = sqrt(dx*dx+dy*dy+dz*dz);
			t = (1.0-*bondDamage)*(c1 * zeta + (1.0-*bondDamage) * OMEGA * alpha * (dY - zeta));
			double fx = t * dx / dY;
			double fy = t * dy / dY;
			double fz = t * dz / dY;

			*(fOwned+0) += fx*cellVolume;
			*(fOwned+1) += fy*cellVolume;
			*(fOwned+2) += fz*cellVolume;
			fInternalOverlap[3*localId+0] -= fx*selfCellVolume;
			fInternalOverlap[3*localId+1] -= fy*selfCellVolume;
			fInternalOverlap[3*localId+2] -= fz*selfCellVolume;
		}

	}
}


void computeInternalForceLinearElastic
(
		const double* xOverlap,
		const double* yOverlap,
		const double* mOwned,
		const double* volumeOverlap,
		const double* dilatationOwned,
		const double* bondDamage,
		double* fInternalOverlap,
		const int*  ownedIDs,
		const int*  localNeighborList,
		int numOwnedPoints,
		double BULK_MODULUS,
		double SHEAR_MODULUS
)
{

	/*
	 * Compute processor local contribution to internal force
	 */
	double K = BULK_MODULUS;
	double MU = SHEAR_MODULUS;
	double OMEGA=1.0;

	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	const double *theta = dilatationOwned;
	double *fOwned = fInternalOverlap;

	const int *neighPtr = localNeighborList;
	double cellVolume, alpha, dx, dy, dz, zeta, dY, t;
	for(int p=0;p<numOwnedPoints;p++){

        int ID = ownedIDs[p];
        xOwned = &xOverlap[3*ID];
        yOwned = &yOverlap[3*ID];
        fOwned = &fInternalOverlap[3*ID];
        m = &mOwned[ID];
        theta = &dilatationOwned[ID];

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		alpha = 15.0*MU/(*m);
		double selfCellVolume = v[p];
		double c1 = OMEGA*(*theta)*(9.0*K-15.0*MU)/(3.0*(*m));
		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			dx = XP[0]-X[0];
			dy = XP[1]-X[1];
			dz = XP[2]-X[2];
			zeta = sqrt(dx*dx+dy*dy+dz*dz);
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			dY = sqrt(dx*dx+dy*dy+dz*dz);
			t = (1.0-*bondDamage)*(c1 * zeta + (1.0-*bondDamage) * OMEGA * alpha * (dY - zeta));
			double fx = t * dx / dY;
			double fy = t * dy / dY;
			double fz = t * dz / dY;

			*(fOwned+0) += fx*cellVolume;
			*(fOwned+1) += fy*cellVolume;
			*(fOwned+2) += fz*cellVolume;
			fInternalOverlap[3*localId+0] -= fx*selfCellVolume;
			fInternalOverlap[3*localId+1] -= fy*selfCellVolume;
			fInternalOverlap[3*localId+2] -= fz*selfCellVolume;
		}

	}
}


void computeInternalForceIsotropicElasticPlastic
(
		const double* xOverlap,
		const double* yOverlap,
		const double* mOwned,
		const double* volumeOverlap,
		const double* dilatationOwned,
		const double* bondDamage,
		const double* dsfOwned,
		const double* deviatoricPlasticExtensionStateN,
		double* deviatoricPlasticExtensionStateNp1,
		const double* lambdaN,
		double* lambdaNP1,
		double* fInternalOverlap,
		const int*  localNeighborList,
		int numOwnedPoints,
		double BULK_MODULUS,
		double SHEAR_MODULUS,
		double HORIZON,
		double yieldStress
)
{

	/*
	 * Compute processor local contribution to internal force
	 */
	double K = BULK_MODULUS;
	double MU = SHEAR_MODULUS;
	double OMEGA=1.0;
	double DELTA=HORIZON;
	/*
	 * 3d variety of yield value
	 */
	double yieldValue = 75.0 * yieldStress * yieldStress / 8 / M_PI / pow(DELTA,5);
	/*
	 * Planar variety of yield value
	 */
//		double THICKNESS=1.0;
//		double yieldValue = 225.0 * yieldStress * yieldStress / 8 / M_PI / THICKNESS / pow(DELTA,4);
//		double yieldValue = 0.5 * pow(15*yieldStress/weightedVol,2) * M_PI * THICKNESS * pow(DELTA,4) / 16.0;



	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	const double *theta = dilatationOwned;
	double *fOwned = fInternalOverlap;

	const int *neighPtr = localNeighborList;
	double cellVolume, alpha, dx, dy, dz, zeta, dY, t, ti, td, ed, edpN, tdTrial;
	for(int p=0;p<numOwnedPoints;p++, xOwned +=3, yOwned +=3, fOwned+=3, m++, theta++, lambdaN++, lambdaNP1++, dsfOwned++){

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		double weightedVol = *m;
		alpha = *dsfOwned * 15.0*MU/weightedVol;
		double selfCellVolume = v[p];
		double c = 3 * K * (*theta) * OMEGA / weightedVol;
		double deltaLambda=0.0;

		/*
		 * Compute norm of trial stress
		 */
		double tdNorm = 0.0;
		tdNorm = computeDeviatoricForceStateNorm(numNeigh,*theta,neighPtr,bondDamage,deviatoricPlasticExtensionStateN,X,Y,xOverlap,yOverlap,v,alpha,OMEGA);

		/*
		 * Evaluate yield function
		 */
		double pointWiseYieldValue = *dsfOwned * (*dsfOwned) * yieldValue;
		double f = tdNorm * tdNorm / 2 - pointWiseYieldValue;
		bool elastic = true;

//		std::cout << "Point id = " << p << std::endl;
//		std::cout << "\tyieldStress/m^(4/5) = " << yieldStress/pow(weightedVol,4/5) << std::endl;
//		std::cout << "\tYield Value = " << yieldValue << "; tdNorm * tdNorm / 2 = " << tdNorm * tdNorm / 2 << std::endl;
		if(f>0){
			/*
			 * This step is incrementally plastic
			 */
			//			std::cout << "\t PLASTIC" << std::endl;
			elastic = false;
			deltaLambda=( tdNorm / sqrt(2.0*pointWiseYieldValue) - 1.0 ) / alpha;
			*lambdaNP1 = *lambdaN + deltaLambda;
		} else {
//			std::cout << "\t ELASTIC" << std::endl;
			*lambdaNP1 = *lambdaN;
		}

		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++, deviatoricPlasticExtensionStateN++, deviatoricPlasticExtensionStateNp1++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			dx = XP[0]-X[0];
			dy = XP[1]-X[1];
			dz = XP[2]-X[2];
			zeta = sqrt(dx*dx+dy*dy+dz*dz);
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			dY = sqrt(dx*dx+dy*dy+dz*dz);
			/*
			 * Deviatoric extension state
			 */
			ed = dY-zeta-*theta*zeta/3;

			/*
			 * Deviatoric plastic extension state from last step
			 */
			edpN = *deviatoricPlasticExtensionStateN;

			/*
			 * Compute trial stress
			 */
			tdTrial = alpha * OMEGA * (ed - edpN);

			/*
			 * Evaluate yield function
			 */
			if(elastic){
				/*
				 * Elastic case
				 */
				td = tdTrial;

				/*
				 * Therefore edpNp1 = edpN
				 */
				*deviatoricPlasticExtensionStateNp1 = *deviatoricPlasticExtensionStateN;

			} else {
				/*
				 * Compute deviatoric force state
				 */
				td = sqrt(2.0*pointWiseYieldValue) * tdTrial / tdNorm;

				/*
				 * Update deviatoric plastic deformation state
				 */
				*deviatoricPlasticExtensionStateNp1 = edpN + td * deltaLambda;

//				std::cout << "Neighbor Id = " << localId << "; Updating deviatoricPlasticExtensionState = " << *deviatoricPlasticExtensionState << std::endl;
			}
//			std::cout << "\tNeighbor Id = " << localId << "\n\ttd = " << td;
			/*
			 * Compute isotropic part of force state
			 */
			ti = c * zeta;

			/*
			 * Force state (with damage)
			 */
			double d=(1.0-*bondDamage);
			t = d*(ti + d*td);

			/*
			 * Assemble pair wise force function
			 */
			double fx = t * dx / dY;
			double fy = t * dy / dY;
			double fz = t * dz / dY;

			*(fOwned+0) += fx*cellVolume;
			*(fOwned+1) += fy*cellVolume;
			*(fOwned+2) += fz*cellVolume;
			fInternalOverlap[3*localId+0] -= fx*selfCellVolume;
			fInternalOverlap[3*localId+1] -= fy*selfCellVolume;
			fInternalOverlap[3*localId+2] -= fz*selfCellVolume;
		}

	}
}


void computeInternalForceIsotropicElasticPlastic
(
		const double* xOverlap,
		const double* yOverlap,
		const double* mOwned,
		const double* volumeOverlap,
		const double* dilatationOwned,
		const double* bondDamage,
		const double* dsfOwned_,
		const double* deviatoricPlasticExtensionStateN,
		double* deviatoricPlasticExtensionStateNp1,
		const double* lambdaN_,
		double* lambdaNP1_,
		double* fInternalOverlap,
		const int*  ownedIDs,
		const int*  localNeighborList,
		int numOwnedPoints,
		double BULK_MODULUS,
		double SHEAR_MODULUS,
		double HORIZON,
		double yieldStress
)
{

	/*
	 * Compute processor local contribution to internal force
	 */
	double K = BULK_MODULUS;
	double MU = SHEAR_MODULUS;
	double OMEGA=1.0;
	double DELTA=HORIZON;
	/*
	 * 3d variety of yield value
	 */
	double yieldValue = 75.0 * yieldStress * yieldStress / 8 / M_PI / pow(DELTA,5);
	/*
	 * Planar variety of yield value
	 */
//		double THICKNESS=1.0;
//		double yieldValue = 225.0 * yieldStress * yieldStress / 8 / M_PI / THICKNESS / pow(DELTA,4);
//		double yieldValue = 0.5 * pow(15*yieldStress/weightedVol,2) * M_PI * THICKNESS * pow(DELTA,4) / 16.0;



	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	const double *theta = dilatationOwned;
	double *fOwned = fInternalOverlap;

	const int *neighPtr = localNeighborList;
	double cellVolume, alpha, dx, dy, dz, zeta, dY, t, ti, td, ed, edpN, tdTrial;
	for(int p=0 ; p<numOwnedPoints ; p++){

        int ID = ownedIDs[p];
        xOwned = &xOverlap[3*ID];
        yOwned = &yOverlap[3*ID];
        fOwned = &fInternalOverlap[3*ID];
        m = &mOwned[ID];
        theta = &dilatationOwned[ID];
        const double* lambdaN = &lambdaN_[ID];
        double* lambdaNP1 = &lambdaNP1_[ID];
        const double* dsfOwned = &dsfOwned_[ID];

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *Y = yOwned;
		double weightedVol = *m;
		alpha = *dsfOwned * 15.0*MU/weightedVol;
		double selfCellVolume = v[p];
		double c = 3 * K * (*theta) * OMEGA / weightedVol;
		double deltaLambda=0.0;

		/*
		 * Compute norm of trial stress
		 */
		double tdNorm = 0.0;
		tdNorm = computeDeviatoricForceStateNorm(numNeigh,*theta,neighPtr,bondDamage,deviatoricPlasticExtensionStateN,X,Y,xOverlap,yOverlap,v,alpha,OMEGA);

		/*
		 * Evaluate yield function
		 */
		double pointWiseYieldValue = *dsfOwned * (*dsfOwned) * yieldValue;
		double f = tdNorm * tdNorm / 2 - pointWiseYieldValue;
		bool elastic = true;

//		std::cout << "Point id = " << p << std::endl;
//		std::cout << "\tyieldStress/m^(4/5) = " << yieldStress/pow(weightedVol,4/5) << std::endl;
//		std::cout << "\tYield Value = " << yieldValue << "; tdNorm * tdNorm / 2 = " << tdNorm * tdNorm / 2 << std::endl;
		if(f>0){
			/*
			 * This step is incrementally plastic
			 */
			//			std::cout << "\t PLASTIC" << std::endl;
			elastic = false;
			deltaLambda=( tdNorm / sqrt(2.0*pointWiseYieldValue) - 1.0 ) / alpha;
			*lambdaNP1 = *lambdaN + deltaLambda;
		} else {
//			std::cout << "\t ELASTIC" << std::endl;
			*lambdaNP1 = *lambdaN;
		}

		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++, deviatoricPlasticExtensionStateN++, deviatoricPlasticExtensionStateNp1++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP = &xOverlap[3*localId];
			const double *YP = &yOverlap[3*localId];
			dx = XP[0]-X[0];
			dy = XP[1]-X[1];
			dz = XP[2]-X[2];
			zeta = sqrt(dx*dx+dy*dy+dz*dz);
			dx = YP[0]-Y[0];
			dy = YP[1]-Y[1];
			dz = YP[2]-Y[2];
			dY = sqrt(dx*dx+dy*dy+dz*dz);
			/*
			 * Deviatoric extension state
			 */
			ed = dY-zeta-*theta*zeta/3;

			/*
			 * Deviatoric plastic extension state from last step
			 */
			edpN = *deviatoricPlasticExtensionStateN;

			/*
			 * Compute trial stress
			 */
			tdTrial = alpha * OMEGA * (ed - edpN);

			/*
			 * Evaluate yield function
			 */
			if(elastic){
				/*
				 * Elastic case
				 */
				td = tdTrial;

				/*
				 * Therefore edpNp1 = edpN
				 */
				*deviatoricPlasticExtensionStateNp1 = *deviatoricPlasticExtensionStateN;

			} else {
				/*
				 * Compute deviatoric force state
				 */
				td = sqrt(2.0*pointWiseYieldValue) * tdTrial / tdNorm;

				/*
				 * Update deviatoric plastic deformation state
				 */
				*deviatoricPlasticExtensionStateNp1 = edpN + td * deltaLambda;

//				std::cout << "Neighbor Id = " << localId << "; Updating deviatoricPlasticExtensionState = " << *deviatoricPlasticExtensionState << std::endl;
			}
//			std::cout << "\tNeighbor Id = " << localId << "\n\ttd = " << td;
			/*
			 * Compute isotropic part of force state
			 */
			ti = c * zeta;

			/*
			 * Force state (with damage)
			 */
			double d=(1.0-*bondDamage);
			t = d*(ti + d*td);

			/*
			 * Assemble pair wise force function
			 */
			double fx = t * dx / dY;
			double fy = t * dy / dY;
			double fz = t * dz / dY;

			*(fOwned+0) += fx*cellVolume;
			*(fOwned+1) += fy*cellVolume;
			*(fOwned+2) += fz*cellVolume;
			fInternalOverlap[3*localId+0] -= fx*selfCellVolume;
			fInternalOverlap[3*localId+1] -= fy*selfCellVolume;
			fInternalOverlap[3*localId+2] -= fz*selfCellVolume;
		}

	}
}

/**
 * Computes norm of deviatoric force state at a particular point
 * @param numNeigh -- number of neighbors at point
 * @param theta    -- dilatation at point
 * @param neighPtr -- list of neighbors at point
 * @param bondDamage     -- damage parameter for each bond at point
 * @param X              -- original coordinates of point
 * @param Y              -- current coordinates of point
 * @param xOverlap       -- pointer to overlap vector of original coordinates; use this to get neighbor original coordinates
 * @param yOverlap       -- pointer to overlap vector of current coordinates; use this to get neighbor current coordinates
 * @param volumeOverlap  -- pointer to volume overlap vector; use this to get volume of neighboring points
 * @param alpha          -- material property (alpha = 15 mu / m
 * @param OMEGA          -- weight function at point
 */
double computeDeviatoricForceStateNorm
(
		int numNeigh,
		double theta,
		const int *neighPtr,
		const double *bondDamage,
		const double *deviatoricPlasticExtensionState,
		const double *X,
		const double *Y,
		const double *xOverlap,
		const double *yOverlap,
		const double *volumeOverlap,
		double alpha,
		double OMEGA
)
{
	double norm=0.0;
	const double *v = volumeOverlap;
	double cellVolume, dx, dy, dz, zeta, dY, ed, edpN, tdTrial;

	for(int n=0;n<numNeigh;n++, neighPtr++, bondDamage++, deviatoricPlasticExtensionState++){
		int localId = *neighPtr;
		cellVolume = v[localId];
		const double *XP = &xOverlap[3*localId];
		const double *YP = &yOverlap[3*localId];
		dx = XP[0]-X[0];
		dy = XP[1]-X[1];
		dz = XP[2]-X[2];
		zeta = sqrt(dx*dx+dy*dy+dz*dz);
		dx = YP[0]-Y[0];
		dy = YP[1]-Y[1];
		dz = YP[2]-Y[2];
		dY = sqrt(dx*dx+dy*dy+dz*dz);

		/*
		 * Deviatoric extension state
		 */
		ed = dY-zeta-theta*zeta/3;

		/*
		 * Deviatoric plastic extension state from last step
		 */
		edpN = *deviatoricPlasticExtensionState;

		/*
		 * Compute trial stress
		 * NOTE: include damage
		 */
		double d=(1.0-*bondDamage);
		tdTrial = d * alpha * OMEGA * (ed - edpN);

		/*
		 * Accumulate norm
		 */
		norm += tdTrial * tdTrial * cellVolume;


//		std::cout << "computeDeviatoricForceStateNorm \n\talpha = " << alpha << "\n\tOMEGA = "<< OMEGA << std::endl;
//		std::cout << "computeDeviatoricForceStateNorm \n\tdY = " << dY <<"\n\tzeta = " << zeta << "\n\ttheta =  "<< theta << std::endl;
//		std::cout << "computeDeviatoricForceStateNorm \n\ted = " << dY-zeta-theta*zeta/3 << "\n\tedpN = " << edpN << std::endl;

	}

	return sqrt(norm);
}


void computeInternalForceViscoelasticStandardLinearSolid
  (
   double delta_t,
   const double *xOverlap,
   const double *yNOverlap,
   const double *yNP1Overlap,
   const double *mOwned,
   const double* volumeOverlap,
   const double* dilatationOwnedN,
   const double* dilatationOwnedNp1,
   const double* bondDamage,
   const double *edbN,
   double *edbNP1,
   double *fInternalOverlap,
   const int*  localNeighborList,
   int numOwnedPoints,
   double BULK_MODULUS,
   double SHEAR_MODULUS,
   double m_tau,
   double m_tau_b
)
{

	/**
	 * Helper function for viscoelastic standard linear solid
	 */
	struct ViscoelasticBetaOperator : public binary_function< double, double, double > {
		ViscoelasticBetaOperator(double tau, double tau_b, double delta_t) : c1(tau_b/tau), c2(tau_b/delta_t)  {}
		double operator() (double deviatoric_extension, double delta_ed) const {
			return c1 * (deviatoric_extension - c2 * delta_ed);
		}
		double c1,c2;
	};

	ViscoelasticBetaOperator beta(m_tau,m_tau_b,delta_t);
	/*
	 * e^{-dt/tau_b}
	 */
	double decay = exp(-1.0/beta.c2);

	/*
	 * Compute processor local contribution to internal force
	 */
	double K = BULK_MODULUS;
	double MU = SHEAR_MODULUS;
	double OMEGA=1.0;

	const double *xOwned = xOverlap;
	const double *yNOwned = yNOverlap;
	const double *yNP1Owned = yNP1Overlap;
	const double *m = mOwned;
	const double *v = volumeOverlap;
	const double *thetaN = dilatationOwnedN;
	const double *thetaNp1 = dilatationOwnedNp1;
	double *fOwned = fInternalOverlap;

	const int *neighPtr = localNeighborList;
	double cellVolume, dx, dy, dz, zeta, dYN, dYNp1, t, ti, td, edN, edNp1, delta_ed, beta_tN, beta_tNp1;
	for(int p=0;p<numOwnedPoints;p++, xOwned +=3, yNOwned +=3, yNP1Owned +=3, fOwned+=3, m++, thetaN++, thetaNp1++){

		int numNeigh = *neighPtr; neighPtr++;
		const double *X = xOwned;
		const double *YN = yNOwned;
		const double *YNP1 = yNP1Owned;
		double weightedVolume = *m;
		double dilatationN   = *thetaN;
		double dilatationNp1 = *thetaNp1;
		double alpha = 15.0*MU/weightedVolume;
		double selfCellVolume = v[p];
		double c = 3.0 * K * dilatationNp1 / weightedVolume;
		for(int n=0;n<numNeigh;n++,neighPtr++,bondDamage++,edbN++,edbNP1++){
			int localId = *neighPtr;
			cellVolume = v[localId];
			const double *XP    = &xOverlap[3*localId];
			const double *YPN   = &yNOverlap[3*localId];
			const double *YPNP1 = &yNP1Overlap[3*localId];
			dx = XP[0]-X[0];
			dy = XP[1]-X[1];
			dz = XP[2]-X[2];
			zeta = sqrt(dx*dx+dy*dy+dz*dz);

			/*
			 * damage state
			 * NOTE: Some additional analysis (pencil and paper) may be required with
			 * regard to handling damage.  The question is where to
			 * apply the damage.  The approach taken here is to
			 * apply damage to the incoming (computed) deviatoric extension state
			 * and evolve the back extension state with the damaged
			 * deviatoric extension state.
			 */
			double damage = (1.0-*bondDamage);

			/*
			 * volumetric scalar state
			 */
			double eiN   = dilatationN * zeta / 3.0;
			double eiNp1 = dilatationNp1 * zeta / 3.0;

			/*
			 * COMPUTE edN
			 */
			dx = YPN[0]-YN[0];
			dy = YPN[1]-YN[1];
			dz = YPN[2]-YN[2];
			dYN = sqrt(dx*dx+dy*dy+dz*dz);
			edN = damage * (dYN - zeta) - eiN;

			/*
			 * COMPUTE edNp1
			 */
			dx = YPNP1[0]-YNP1[0];
			dy = YPNP1[1]-YNP1[1];
			dz = YPNP1[2]-YNP1[2];
			dYNp1 = sqrt(dx*dx+dy*dy+dz*dz);
			edNp1 = damage * (dYNp1 - zeta) - eiNp1;

			delta_ed = edNp1-edN;
			beta_tN = beta(edN,delta_ed);
			beta_tNp1 = beta(edNp1,delta_ed);
			/*
			 * integrate back extension state forward in time
			 */
			*edbNP1 = ( (*edbN) - beta_tN ) * decay + beta_tNp1;

			/*
			 * compute deviatoric force state
			 */
			td = alpha * OMEGA * ( edNp1 - *edbNP1 );

			/*
			 * Compute volumetric force state
			 */
			ti = c * OMEGA * zeta;

			/*
			 * Note that damage has already been applied once to 'td' (through ed) above.
			 */
			t = damage * (ti + td);
			double fx = t * dx / dYNp1;
			double fy = t * dy / dYNp1;
			double fz = t * dz / dYNp1;

			*(fOwned+0) += fx*cellVolume;
			*(fOwned+1) += fy*cellVolume;
			*(fOwned+2) += fz*cellVolume;
			fInternalOverlap[3*localId+0] -= fx*selfCellVolume;
			fInternalOverlap[3*localId+1] -= fy*selfCellVolume;
			fInternalOverlap[3*localId+2] -= fz*selfCellVolume;
		}
	}

}


/**
 * Call this function on a single point 'X'
 * NOTE: neighPtr to should point to 'numNeigh' for 'X'
 * and thus describe the neighborhood list as usual
 * NOTE: this function will overwrite entries in 'yOverlap'
 * for all of 'X' neighbors
 * OUTPUT: yOverlap such that there is a state of pure
 * shear at 'X'
 */
void set_pure_shear
(
		const int *neighPtr,
		const double *X,
		const double *xOverlap,
		double *yOverlap,
		PURE_SHEAR mode,
		double gamma
)
{

	/*
	 * Pure shear centered at X
	 * X has no displacement
	 */
	int numNeigh=*neighPtr; neighPtr++;
	for(int n=0;n<numNeigh;n++, neighPtr++){
		int localId = *neighPtr;
		const double *XP = &xOverlap[3*localId];

		double dx = XP[0]-X[0];
		double dy = XP[1]-X[1];
		double dz = XP[2]-X[2];

		/*
		 * Pure shear
		 */
		double zx(0.0), xy(0.0), yz(0.0);
		switch(mode){
		case ZX:
			zx = gamma * dx;
			break;
		case XY:
			xy = gamma * dy;
			break;
		case YZ:
			yz = gamma * dz;
			break;
		}

		double *YP = &yOverlap[3*localId];
		YP[0] = XP[0] + xy;
		YP[1] = XP[1] + yz;
		YP[2] = XP[2] + zx;

	}

}


double computeWeightedVolume
(
		const double *X,
		const double *xOverlap,
		const double* volumeOverlap,
		const int* localNeighborList
){

	double m=0.0;
	double cellVolume;
	const int *neighPtr = localNeighborList;
	int numNeigh = *neighPtr; neighPtr++;
//	std::cout << NAMESPACE <<"computeWeightedVolume\n";
//	std::cout << "\tnumber of neighbors = " << numNeigh << std::endl;
	for(int n=0;n<numNeigh;n++,neighPtr++){
		int localId = *neighPtr;
		cellVolume = volumeOverlap[localId];
		const double *XP = &xOverlap[3*localId];
		double dx = XP[0]-X[0];
		double dy = XP[1]-X[1];
		double dz = XP[2]-X[2];
		m+=(dx*dx+dy*dy+dz*dz)*cellVolume;
	}

	return m;
}

/**
 * Call this function on a single point 'X'
 * NOTE: neighPtr to should point to 'numNeigh' for 'X'
 * and thus describe the neighborhood list as usual
 */
double computeDilatation
(
		const int *neighPtr,
		const double *X,
		const double *xOverlap,
		const double *Y,
		const double *yOverlap,
		const double *volumeOverlap,
		double weightedVolume
)
{
	double OMEGA=1.0;
	double bondDamage=0.0;
	const double *v = volumeOverlap;
	double m = weightedVolume;
	double theta = 0.0;
	int numNeigh=*neighPtr; neighPtr++;
	for(int n=0;n<numNeigh;n++,neighPtr++){

		int localId = *neighPtr;
		double cellVolume = v[localId];

		const double *XP = &xOverlap[3*localId];
		double dx = XP[0]-X[0];
		double dy = XP[1]-X[1];
		double dz = XP[2]-X[2];
		double zetaSqared = dx*dx+dy*dy+dz*dz;

		const double *YP = &yOverlap[3*localId];
		dx = YP[0]-Y[0];
		dy = YP[1]-Y[1];
		dz = YP[2]-Y[2];
		double dY = dx*dx+dy*dy+dz*dz;
		double d = sqrt(zetaSqared);
		double e = sqrt(dY)-d;
		theta += 3.0*OMEGA*(1.0-bondDamage)*d*e*cellVolume/m;

	}
	return theta;
}


void computeShearCorrectionFactor
(
		int numOwnedPoints,
		const double *xOverlap,
		double *yOverlap_scratch_required_work_space,
		const double *volumeOverlap,
		const double *owned_weighted_volume,
		const int*  localNeighborList,
		double horizon,
		double *shearCorrectionFactorOwned
){
	double gamma=1.0e-6;
	double reference = 4.0 * M_PI * gamma * gamma * pow(horizon,5) / 75.0;
	const int *neighPtr = localNeighborList;
	const double *xOwned = xOverlap;
	const double *yOwned = yOverlap_scratch_required_work_space;
	double *yOverlap = yOverlap_scratch_required_work_space;
	double *scaleFactor = shearCorrectionFactorOwned;
	PURE_SHEAR mode;
	for(int p=0;p<numOwnedPoints;p++, xOwned+=3, yOwned+=3, scaleFactor++, owned_weighted_volume++){
		int numNeigh = *neighPtr;
		const double *X = xOwned;
		const double *Y = yOwned;
		double m = *owned_weighted_volume;
		double dsf, max_dsf;

		mode = XY;
		set_pure_shear(neighPtr,xOwned,xOverlap,yOverlap,mode,gamma);
		dsf=compute_norm_2_deviatoric_extension(neighPtr,X,xOverlap,Y,yOverlap,volumeOverlap,m);
		max_dsf=dsf;

		mode = ZX;
		set_pure_shear(neighPtr,xOwned,xOverlap,yOverlap,mode,gamma);
		dsf=compute_norm_2_deviatoric_extension(neighPtr,X,xOverlap,Y,yOverlap,volumeOverlap,m);
		if(dsf>max_dsf) max_dsf = dsf;

		mode = YZ;
		set_pure_shear(neighPtr,xOwned,xOverlap,yOverlap,mode,gamma);
		dsf=compute_norm_2_deviatoric_extension(neighPtr,X,xOverlap,Y,yOverlap,volumeOverlap,m);
		if(dsf>max_dsf) max_dsf = dsf;

		/*
		 * Better guard against division by zero
		 *
		 */
		double tolerance(1.0e-15);
		if(max_dsf/reference < tolerance)
			max_dsf=1.0;
		else
			max_dsf = reference/max_dsf;

		*scaleFactor = max_dsf;
		neighPtr+=(numNeigh+1);
	}

}

double compute_norm_2_deviatoric_extension
(
		const int *neighPtr,
		const double *X,
		const double *xOverlap,
		const double *Y,
		const double *yOverlap,
		const double *volumeOverlap,
		double weighted_volume
)
{

	const double *v = volumeOverlap;
	double cellVolume, dx, dy, dz, zeta, dY, ed;

	/*
	 * Compute weighted volume
	 */
	double m = weighted_volume;
//	double m = computeWeightedVolume(X,xOverlap,volumeOverlap,neighPtr);
//	std::cout << NAMESPACE << "probeShearModulusScaleFactor weighted volume = " << m << std::endl;

	/*
	 * Compute dilatation
	 */
	double theta = computeDilatation(neighPtr,X,xOverlap,Y,yOverlap,volumeOverlap,m);
//	std::cout << NAMESPACE << "probeShearModulusScaleFactor theta = " << theta << std::endl;

	/*
	 * Pure shear centered at X
	 * X has no displacement
	 */
	double ed_squared=0.0;
	int numNeigh=*neighPtr; neighPtr++;
	for(int n=0;n<numNeigh;n++, neighPtr++){
		int localId = *neighPtr;
		cellVolume = v[localId];
		const double *XP = &xOverlap[3*localId];
		dx = XP[0]-X[0];
		dy = XP[1]-X[1];
		dz = XP[2]-X[2];
		zeta = sqrt(dx*dx+dy*dy+dz*dz);

		/*
		 * Deformation State
		 */
		const double *YP = &yOverlap[3*localId];
		dx = YP[0]-Y[0];
		dy = YP[1]-Y[1];
		dz = YP[2]-Y[2];
		dY = sqrt(dx*dx+dy*dy+dz*dz);

		/*
		 * Deviatoric extension state
		 */
		ed = dY-zeta-theta*zeta/3;

		/*
		 * Accumulate norm
		 */
		ed_squared += ed * ed * cellVolume;

	}

	return ed_squared;
}

}
