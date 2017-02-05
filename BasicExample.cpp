

#include "BasicExample.h"

#include "btBulletDynamicsCommon.h"
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Z 5

#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include "../CommonInterfaces/CommonRigidBodyBase.h"
#include "NeuralNetwork.h"
#include "Vector2D.h"
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#define NUM_TRAIN 1000000 // Use 1000000

int num = 0;
short collisionFilterGroup = short(btBroadphaseProxy::CharacterFilter);
short collisionFilterMask = short(btBroadphaseProxy::AllFilter ^ (btBroadphaseProxy::CharacterFilter));
static btScalar radius(0.2);

struct BasicExample : public CommonRigidBodyBase
{
	int j;
	double pos_x_, pos_y_, pos_z_;
	double target_x_, target_y_, target_z_;
	int num_data=0;
	int handled = 1;
	NeuralNetwork nn_;
	btScalar distance_;
	bool m_once;
	double distance;
	btAlignedObjectArray<btJointFeedback*> m_jointFeedback;
	btHingeConstraint* hinge_shader;
	btHingeConstraint* hinge_elbow;
	btRigidBody* linkBody;
	btVector3 groundOrigin_target;
	btRigidBody* body;
	btRigidBody* human_body;
	BasicExample(struct GUIHelperInterface* helper);
	virtual ~BasicExample();
	virtual void initPhysics();

	void updateSubstep(const bool print = false);
	void moveTarget();

	virtual void stepSimulation(float deltaTime);
	void lockLiftHinge(btHingeConstraint* hinge);

	virtual bool keyboardCallback(int key, int state);
	virtual void resetCamera()
	{

		float dist = 5;
		float pitch = 270;
		float yaw = 21;
		float targetPos[3] = { -1.34,3.4,-0.44 };
		m_guiHelper->resetCamera(dist, pitch, yaw, targetPos[0], targetPos[1], targetPos[2]);
	}
};

BasicExample::BasicExample(struct GUIHelperInterface* helper)
	:CommonRigidBodyBase(helper), nn_(2 + 2 + 0, 4, 1),
	m_once(true)
{
}
BasicExample::~BasicExample()
{
	for (int i = 0; i<m_jointFeedback.size(); i++)
	{
		delete m_jointFeedback[i];
	}

}


void BasicExample::stepSimulation(float deltaTime)
{
	if (0)//m_once)
	{
		m_once = false;
		btHingeConstraint* hinge = (btHingeConstraint*)m_dynamicsWorld->getConstraint(0);

		btRigidBody& bodyA = hinge->getRigidBodyA();
		btTransform trA = bodyA.getWorldTransform();
		btVector3 hingeAxisInWorld = trA.getBasis()*hinge->getFrameOffsetA().getBasis().getColumn(2);
		hinge->getRigidBodyA().applyTorque(-hingeAxisInWorld * 10);
		hinge->getRigidBodyB().applyTorque(hingeAxisInWorld * 10);

	}

	//collison check
	int numManifolds = m_dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold = m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = contactManifold->getBody0();
		const btCollisionObject* obB = contactManifold->getBody1();

		int numContacts = contactManifold->getNumContacts();
		for (int j = 0; j < numContacts; j++)
		{
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() < 0.f)
			{
				const btVector3& ptA = pt.getPositionWorldOnA();
				const btVector3& ptB = pt.getPositionWorldOnB();
				const btVector3& normalOnB = pt.m_normalWorldOnB;
				b3Printf("check\n");
			}
		}
	}
	//moveTarget();
	//std::cout << "check" << std::endl;



		
	updateSubstep(false);
	
	//pos_z_ = linkBody->getCenterOfMassPosition().getZ();
	//pos_y_ = linkBody->getCenterOfMassPosition().getY();
	//target_z_ = body->getCenterOfMassPosition().getZ();
	//target_y_ = body->getCenterOfMassPosition().getY();

	//distance_ = (Vector2D<double>(pos_y_, pos_z_) - Vector2D<double>(target_y_, target_z_)).getMagnitude();
	//b3Printf("distance = %f\n", distance_);

	//get distance
	/*distance = sqrt(pow((body->getCenterOfMassPosition().getZ() - linkBody->getCenterOfMassPosition().getZ()), 2) + pow((body->getCenterOfMassPosition().getY() - linkBody->getCenterOfMassPosition().getY()), 2)) - 0.225;*/
	/*moveTarget();
	distance = sqrt(pow((body->getCenterOfMassPosition().getZ() - linkBody->getCenterOfMassPosition().getZ()), 2) + pow((body->getCenterOfMassPosition().getY() - linkBody->getCenterOfMassPosition().getY()), 2)) - 0.225;
	b3Printf("distance = %f", distance);
	b3Printf("\n");*/
	m_dynamicsWorld->stepSimulation(1. / 240, 0);

	static int count = 0;

}

void initState(BasicExample* target) {
	target->m_guiHelper->removeAllGraphicsInstances();
	target->initPhysics();
}

void moveLeft(btHingeConstraint *target) {
	target->setLimit(-M_PI / 1.2f, M_PI / 1.2f);
	target->enableAngularMotor(true, -15.0, 4000.f);

}

void moveRight(btHingeConstraint *target) {
	target->setLimit(-M_PI / 1.2f, M_PI / 1.2f);
	target->enableAngularMotor(true, 15.0, 4000.f);
}

void BasicExample::initPhysics()
{
	int upAxis = 1;
	m_guiHelper->setUpAxis(upAxis);

	createEmptyDynamicsWorld();
	m_dynamicsWorld->getSolverInfo().m_splitImpulse = false;

	m_dynamicsWorld->setGravity(btVector3(0, 0, -10));

	m_guiHelper->createPhysicsDebugDrawer(m_dynamicsWorld);
	int mode = btIDebugDraw::DBG_DrawWireframe
		+ btIDebugDraw::DBG_DrawConstraints
		+ btIDebugDraw::DBG_DrawConstraintLimits;
	m_dynamicsWorld->getDebugDrawer()->setDebugMode(mode);


	{ // create a door using hinge constraint attached to the world

		int numLinks = 3;
		bool spherical = false;					//set it ot false -to use 1DoF hinges instead of 3DoF sphericals
		bool canSleep = false;
		bool selfCollide = false;

		btVector3 baseHalfExtents(0.05, 0.37, 0.1);
		btVector3 linkHalfExtents(0.05, 0.37, 0.1);

		btBoxShape* baseBox = new btBoxShape(baseHalfExtents);
		btVector3 basePosition = btVector3(-0.4f, 4.f, 0.f);
		btTransform baseWorldTrans;
		baseWorldTrans.setIdentity();
		baseWorldTrans.setOrigin(basePosition);

		//mbC->forceMultiDof();							//if !spherical, you can comment this line to check the 1DoF algorithm
		//init the base
		btVector3 baseInertiaDiag(0.f, 0.f, 0.f);
		float baseMass = 0.f;
		float linkMass = 1.f;

		btRigidBody* base = createRigidBody(baseMass, baseWorldTrans, baseBox);
		m_dynamicsWorld->removeRigidBody(base);
		base->setDamping(0, 0);
		m_dynamicsWorld->addRigidBody(base, collisionFilterGroup, collisionFilterMask);
		btBoxShape* linkBox1 = new btBoxShape(linkHalfExtents);
		btBoxShape* linkBox2 = new btBoxShape(linkHalfExtents);
		btSphereShape* linkSphere = new btSphereShape(radius);

		btRigidBody* prevBody = base;

		for (int i = 0; i<numLinks; i++)
		{
			btTransform linkTrans;
			linkTrans = baseWorldTrans;

			linkTrans.setOrigin(basePosition - btVector3(0, linkHalfExtents[1] * 2.f*(i + 1), 0));

			btCollisionShape* colOb = 0;

			if (i == 0)
			{
				colOb = linkBox1;
			}
			else if (i == 1)
			{
				colOb = linkBox2;
			}
			else
			{
				colOb = linkSphere;
			}
			linkBody = createRigidBody(linkMass, linkTrans, colOb);
			m_dynamicsWorld->removeRigidBody(linkBody);
			m_dynamicsWorld->addRigidBody(linkBody, collisionFilterGroup, collisionFilterMask);
			linkBody->setDamping(0, 0);
			btTypedConstraint* con = 0;

			if (i == 0)
			{
				//create a hinge constraint
				btVector3 pivotInA(0, -linkHalfExtents[1], 0);
				btVector3 pivotInB(0, linkHalfExtents[1], 0);
				btVector3 axisInA(1, 0, 0);
				btVector3 axisInB(1, 0, 0);
				bool useReferenceA = true;
				hinge_shader = new btHingeConstraint(*prevBody, *linkBody,
					pivotInA, pivotInB,
					axisInA, axisInB, useReferenceA);
				hinge_shader->setLimit(0.0f, 0.0f);
				m_dynamicsWorld->addConstraint(hinge_shader, true);
				con = hinge_shader;
			}
			else if (i == 1)
			{
				//create a hinge constraint
				btVector3 pivotInA(0, -linkHalfExtents[1], 0);
				btVector3 pivotInB(0, linkHalfExtents[1], 0);
				btVector3 axisInA(1, 0, 0);
				btVector3 axisInB(1, 0, 0);
				bool useReferenceA = true;
				hinge_elbow = new btHingeConstraint(*prevBody, *linkBody,
					pivotInA, pivotInB,
					axisInA, axisInB, useReferenceA);
				hinge_elbow->setLimit(0.0f, 0.0f);
				m_dynamicsWorld->addConstraint(hinge_elbow, true);
				con = hinge_elbow;
			}
			else
			{
				btTransform pivotInA(btQuaternion::getIdentity(), btVector3(0, -radius, 0));						//par body's COM to cur body's COM offset
				btTransform pivotInB(btQuaternion::getIdentity(), btVector3(0, radius, 0));							//cur body's COM to cur body's PIV offset
				btGeneric6DofSpring2Constraint* fixed = new btGeneric6DofSpring2Constraint(*prevBody, *linkBody,
					pivotInA, pivotInB);
				fixed->setLinearLowerLimit(btVector3(0, 0, 0));
				fixed->setLinearUpperLimit(btVector3(0, 0, 0));
				fixed->setAngularLowerLimit(btVector3(0, 0, 0));
				fixed->setAngularUpperLimit(btVector3(0, 0, 0));

				con = fixed;

			}
			btAssert(con);
			if (con)
			{
				btJointFeedback* fb = new btJointFeedback();
				m_jointFeedback.push_back(fb);
				con->setJointFeedback(fb);

				m_dynamicsWorld->addConstraint(con, true);
			}
			prevBody = linkBody;

		}

	}

	if (1)
	{
		/*btVector3 groundHalfExtents(0.4, 0.0, 0.025);
		groundHalfExtents[upAxis] = 0.4f;
		btBoxShape* box = new btBoxShape(groundHalfExtents);
		box->initializePolyhedralFeatures();*/

		btSphereShape* linkSphere_1 = new btSphereShape(radius);

		btTransform start; start.setIdentity();
		groundOrigin_target = btVector3(-0.4f, 4.0f, -1.6f);

		start.setOrigin(groundOrigin_target);
		body = createRigidBody(0, start, linkSphere_1);

		body->setFriction(0);



		/*btVector3 human_HalfExtents(0.8, 0.0, 0.025);
		human_HalfExtents[upAxis] = 0.8f;
		btBoxShape* human_box = new btBoxShape(human_HalfExtents);
		human_box->initializePolyhedralFeatures();

		btTransform human_start;
		human_start.setIdentity();
		groundOrigin_target = btVector3(-0.4f, 2.8f, -1.6f);

		human_start.setOrigin(groundOrigin_target);
		human_body = createRigidBody(0, human_start, human_box);

		human_body->setFriction(0);*/

	}

	pos_z_ = linkBody->getCenterOfMassPosition().getZ();
	pos_y_ = linkBody->getCenterOfMassPosition().getY();
	target_z_ = body->getCenterOfMassPosition().getZ();
	target_y_ = body->getCenterOfMassPosition().getY();

	distance_ = (Vector2D<double>(pos_y_, pos_z_) - Vector2D<double>(target_y_, target_z_)).getMagnitude();
	m_guiHelper->autogenerateGraphicsObjects(m_dynamicsWorld);
}


void BasicExample::updateSubstep(const bool print)
{
	VectorND<D> input;
	input.initialize(4);

	input[0] = pos_z_;
	input[1] = pos_y_;
	input[2] = target_y_;
	input[3] = target_z_;

	nn_.setInputVector(input);
	nn_.feedForward();

	VectorND<D> output;
	nn_.copyOutputVector(output, false);
	
	const int selected_dir = nn_.getIXProbabilistic(output);
	
	
	

	if (handled) {
		j = selected_dir;
		switch (selected_dir)
		{

		case 0:
		{
			// b3Printf("left.\n");

			
			moveRight(hinge_shader);
			handled = 0;
			//b3Printf("second = %f\n", linkBody->getCenterOfMassPosition().getZ());
			break;

		}
		case 1:
		{
			// b3Printf("right.\n");

			moveLeft(hinge_shader);
			handled = 0;
			break;
		}
		case 2:
		{

			// b3Printf("left.\n");
			moveLeft(hinge_elbow);
			handled = 0;
			/*btVector3 basePosition = btVector3(0.0, 0.0f, 1.54f);
			body->translate(basePosition);*/

			break;

		}
		case 3:
		{

			// b3Printf("left.\n");
			moveRight(hinge_elbow);
			handled = 0;
			/*btVector3 basePosition = btVector3(0.0, 0.0f, -1.54f);
			body->translate(basePosition);*/
			break;
		}
		}
	}
	else
	{

		switch (j)
		{
		case 0:
		case 1:
		{
			
			lockLiftHinge(hinge_shader);
			handled = 1;
			break;
		}
		case 2:
		case 3:
		{
			
			lockLiftHinge(hinge_elbow);
			handled = 1;
			break;
		}
		default:

			break;
		}
	}
	
	pos_z_ = linkBody->getCenterOfMassPosition().getZ();
	pos_y_ = linkBody->getCenterOfMassPosition().getY();
	
	//b3Printf("distance = %f\n", pos_z_);
	const double new_distance_ = (Vector2D<double>(pos_y_, pos_z_) - Vector2D<double>(target_y_, target_z_)).getMagnitude();
	
	
	double reward_value = distance_ - new_distance_;
	
	
	if (new_distance_ <0.5)
	{
		distance_ = new_distance_;

		moveTarget(); 

		return; 
	}

	VectorND<double> reward_vector(output); 

	for (int d = 0; d < reward_vector.num_dimension_; d++)
	{
		if (selected_dir == d)
		{
			reward_vector[d] = reward_value > 0 ? 0.999 : 0.001;
		}
		else
		{
			reward_vector[d] = reward_vector[d] < 0.001 ? 0.001 : reward_vector[d];
		}
	}

	const int max_tr = NUM_TRAIN;

	static int counter = 0;
	const int one_percent = (double)max_tr / 100.0;

	static int itr = 0;
	if (itr < max_tr) // train
	{
		itr++;
		counter++;

		if (counter == one_percent)
		{
			printf("%f percent \n", (double)itr / (double)max_tr * 100.0);

			counter = 0;
		}

		nn_.propBackward(reward_vector);
	}

	distance_ = new_distance_;
}

void BasicExample::moveTarget()
{

	switch (num_data) {
	case 0:
	{
		btVector3 basePosition1 = btVector3(0.0, -1.4, 0.0);
		body->translate(basePosition1);
		num_data = 1;
		break;
	}

	case 1:
	{
		btVector3 basePosition2 = btVector3(0.0, 0.0, 3.08);
		body->translate(basePosition2);
		num_data = 2;
		break;
	}

	case 2:
	{
		btVector3 basePosition3 = btVector3(0.0, 1.4, 0.0);
		body->translate(basePosition3);
		num_data = 3;
		break;
	}

	case 3:
	{
		btVector3 basePosition4 = btVector3(0.0, 0.0, -3.08);
		body->translate(basePosition4);
		num_data = 0;
		break;
	}

	}

	target_y_ = body->getCenterOfMassPosition().getY();
	target_z_ = body->getCenterOfMassPosition().getZ();
	//std::cout << target_y_ << std::endl;
}
bool BasicExample::keyboardCallback(int key, int state)
{
	bool handled = true;
	if (state)
	{
		switch (key)
		{

		case B3G_HOME:
		{
			// b3Printf("Rest.\n");
			moveTarget();
			//initState(this);
			break;
		}
		case B3G_LEFT_ARROW:
		{

			// b3Printf("left.\n");
			moveLeft(hinge_shader);
			handled = true;
			break;

		}
		case B3G_RIGHT_ARROW:
		{
			// b3Printf("right.\n");
			moveRight(hinge_shader);
			handled = true;
			break;
		}
		case B3G_UP_ARROW:
		{
			// b3Printf("left.\n");
			moveLeft(hinge_elbow);
			handled = true;
			/*btVector3 basePosition = btVector3(0.0, 0.0f, 1.54f);
			body->translate(basePosition);*/

			break;

		}
		case B3G_DOWN_ARROW:
		{
			// b3Printf("left.\n");
			moveRight(hinge_elbow);
			handled = true;
			/*btVector3 basePosition = btVector3(0.0, 0.0f, -1.54f);
			body->translate(basePosition);*/
			break;
		}
		}
	}
	else
	{
		switch (key)
		{

		case B3G_LEFT_ARROW:
		case B3G_RIGHT_ARROW:
		{

			lockLiftHinge(hinge_shader);
			handled = true;
			break;
		}
		case B3G_UP_ARROW:
		case B3G_DOWN_ARROW:
		{

			lockLiftHinge(hinge_elbow);
			handled = true;
			break;
		}
		default:

			break;
		}
	}
	return handled;
}

void BasicExample::lockLiftHinge(btHingeConstraint* hinge)
{
	btScalar hingeAngle = hinge->getHingeAngle();
	btScalar lowLim = hinge->getLowerLimit();
	btScalar hiLim = hinge->getUpperLimit();
	hinge->enableAngularMotor(false, 0, 0);

	if (hingeAngle < lowLim)
	{
		//		m_liftHinge->setLimit(lowLim, lowLim + LIFT_EPS);
		hinge->setLimit(lowLim, lowLim);
	}
	else if (hingeAngle > hiLim)
	{
		//		m_liftHinge->setLimit(hiLim - LIFT_EPS, hiLim);
		hinge->setLimit(hiLim, hiLim);
	}
	else
	{
		//		m_liftHinge->setLimit(hingeAngle - LIFT_EPS, hingeAngle + LIFT_EPS);
		hinge->setLimit(hingeAngle, hingeAngle);
	}
	return;
}


CommonExampleInterface*    BasicExampleCreateFunc(CommonExampleOptions& options)
{
	return new BasicExample(options.m_guiHelper);

}


B3_STANDALONE_EXAMPLE(BasicExampleCreateFunc)
