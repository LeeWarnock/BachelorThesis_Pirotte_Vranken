//==============================================================================
/*
<<<<<<< HEAD
Implemented by Casper Vranken and Niels Pirotte commisioned by the University of Hasselt, Belgium
=======
Implemented by Casper Vranken and Niels Pirotte commisioned by the University of Hasselt and KU Leuven, Belgium
>>>>>>> origin/IST

Algorithm sources: 
-new geometric datastructures for collision detection and haptics - Ren� Weller
*/
//==============================================================================

//------------------------------------------------------------------------------
#ifndef CollisionDetectionAlgorithmsH
#define CollisionDetectionAlgorithmsH
//------------------------------------------------------------------------------
#include "collisions/CCollisionBasics.h"
#include "collisions/CCollisionAABB.h"
#include "collisions/CCollisionAABBTree.h"
#include "collisions/CCollisionAABBBox.h"
#include "collisions/CGenericCollision.h"
#include "ist/Sphere.h"
#include "ist/InnerSphereTree.h"
#include "math/CMaths.h"
#include <iostream>
#include <limits>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace chai3d {
	//------------------------------------------------------------------------------

	//==============================================================================
	/*!
	\file  CollisionDetectionAlgorithms.h

	\brief
	Implements the algoritms for traversing a BVH-tree
	*/
	//==============================================================================

	//==============================================================================

	//traverse an AABB tree with the following algorithm:
	//algorithm page 121 in book:new geometric datastructures for collision detection and haptics
	//////////////////////////////////////////////////////////////////////
	//Algorithm 5.2 checkDistance(A, B, minDist)
	//input : A, B = spheres in the inner sphere tree
	//	in / out : minDist = overall minimum distance seen so far
	//	if A and B are leaves then
	//		// end of recursion
	//		minDist = min{ distance(A,B), minDist }
	//	else
	//		// recursion step
	//		forall children a[i] of A do
	//		forall children b[j] of B do
	//		if distance(a[i], b[j]) < minDist then
	//			checkDistance(a[i], b[j], minDist)
	//////////////////////////////////////////////////////////////////////

	//De waarde van mindist gaat recursief worden aangepast door deze functie.
	//Dit is voor AABB
	void checkDistance(cCollisionAABBNode* A,
		cCollisionAABBNode* B,
		double &mindist,
		cCollisionAABB* tree_A,
		cCollisionAABB* tree_B,
		int maxdiepte,
		int &huidigeDiepte,
		cVector3d myLocal,
		cVector3d BLocal,
		cVector3d& positie)
	{
		if (mindist == 0.0) return; //We only want to know if the objects are colliding or not
		if ((A->m_nodeType == cAABBNodeType::C_AABB_NODE_LEAF && B->m_nodeType == cAABBNodeType::C_AABB_NODE_LEAF) || A->m_depth == maxdiepte) {
			mindist = cMin(mindist, A->m_bbox.distance(&(B->m_bbox), myLocal, BLocal));
			if (mindist == 0.0f) {
				positie = (A->m_bbox.getCenter() + myLocal);
			}
			//std::cout << mindist << " min" << std::endl;
		}
		else {
			//recursion
			std::vector<cCollisionAABBNode> children_A = tree_A->getChildren(A);
			std::vector<cCollisionAABBNode> children_B = tree_B->getChildren(B);

			for (int i = 0; i < children_A.size(); i++) {
				for (int j = 0; j < children_B.size(); j++) {
					cCollisionAABBNode newA = children_A[i];
					cCollisionAABBNode newB = children_B[j];
					float afstand = newA.m_bbox.distance(&(newB.m_bbox), myLocal, BLocal);
					if ((afstand < mindist)&&(afstand == 0.0f)) checkDistance(&newA, &newB, mindist, tree_A, tree_B, maxdiepte, huidigeDiepte, myLocal, BLocal, positie);
				}
			}
		}
		//std::cout << std::endl;
	}

	/*
		This function calculates the minimum distance between two spheres while taking into account their children.
		This is done using recursion and is used for inner sphere trees.

		\param spheraA			The first sphere to check.
		\param sphereB			The second sphere to check.
		\param tree1			The tree of the first sphere.
		\param tree2			The tree of the second sphere.
		\param maxdiepte		The maximum depth the algorithm is allowed to check.
		\param huidigeDiepte	The current depth of the checking algorithm.
	*/
	float checkDistance(
		Sphere* sphereA,
		Sphere* sphereB,
		InnerSphereTree* tree1,
		InnerSphereTree* tree2,
		int maxdiepte,
		bool& stop) {

		// Return the distance between 2 spheres when we went deeper in the tree than specified.
		// This results in worse accuracy and should ideally never be called.
		if ((sphereA->getDepth() > maxdiepte)||(sphereB->getDepth()>maxdiepte)) return sphereA->distance(sphereB, tree1->getRootSphere()->getPosition(), tree2->getRootSphere()->getPosition());

		// Calculate the distance between the 2 spheres. 
		// If the distance is greater than 0, The 2 spheres do not collide and the check should finish here.
		float afstand = sphereA->distance(sphereB, tree1->getRootSphere()->getPosition(), tree2->getRootSphere()->getPosition());
		if (afstand > 0) return afstand;

		// If the 2 spheres collide and we are not at the maximum depth yet, continue the recursion.
		afstand = std::numeric_limits<float>::infinity();

		// Every sphere can have multiple children.
		std::vector<Sphere*> children_A = sphereA->getChildren();
		std::vector<Sphere*> children_B = sphereB->getChildren();

		// Iterate through all of the 2 sphere's children.
		std::vector<Sphere*>::iterator itA, itB;
		itA = children_A.begin(); itB = children_B.begin();
		for (itA; itA < children_A.end(); itA++) {
			for (itB; itB < children_B.end(); itB++) {
				Sphere* newA = (*itA);
				Sphere* newB = (*itB);
				
				if (stop) goto einde;

				// If the children aren't leaf nodes, continue the recursion.
				if ((newA->getState() != sphereState::SPHERE_LEAF) && (newB->getState() != sphereState::SPHERE_LEAF)) {
					afstand = cMin(checkDistance(newA, newB, tree1, tree2, maxdiepte, stop), afstand);
				}
				// If both children are leaf nodes, calculate the distance.
				// If the distance is 0 or smaller, a collision has occured and the distance should be returned.
				else if ((newA->getState() == sphereState::SPHERE_LEAF) && (newB->getState() == sphereState::SPHERE_LEAF)) {
					afstand = newA->distance(newB, tree1->getRootSphere()->getPosition(), tree2->getRootSphere()->getPosition());
					if (afstand <= 0) {
						stop = true;
						goto einde;
					}
				}
				else if (newB->getState() == sphereState::SPHERE_LEAF) {
					afstand = cMin(checkDistance(newA, sphereB, tree1, tree2, maxdiepte, stop), afstand);
				}
				else if (newA->getState() == sphereState::SPHERE_LEAF) {
					afstand = cMin(checkDistance(sphereA, newB, tree1, tree2, maxdiepte, stop), afstand);
				}
			}
		}

		einde: return afstand;
	}

	//Help functions
	/////////////////////////////////////////////////////////////////////////////////

	//------------------------------------------------------------------------------
} // namespace chai3d
  //------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
#endif
  //------------------------------------------------------------------------------
