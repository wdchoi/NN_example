#include "BasicExample.h"

#include "btBulletDynamicsCommon.h"
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Z 5

#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include "CommonInterfaces/CommonRigidBodyBase.h"
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#define NUM_TRAIN 1000000 // Use 1000000
#include <iostream>

#define MAX2(a, b)							((a) > (b) ? (a) : (b))
#define MAX3(a, b, c)						(MAX2(MAX2(a, b), (c)))
#define MAX4(a, b, c, d)					(MAX2(MAX3(a, b, c), (d)))

class CellData
{
public:
	double q_[4] = { 0.0, 0.0, 0.0, 0.0 }; // up, down, left, right
	double reward_ = 0.0;

	CellData()
	{}

	double getMaxQ()
	{
		return MAX4(q_[0], q_[1], q_[2], q_[3]);
	}
	int getMaxQ_Num()
	{
		if (q_[0] == this->getMaxQ())
			return 0;
		if (q_[1] == this->getMaxQ())
			return 1;
		if (q_[2] == this->getMaxQ())
			return 2;
		if (q_[3] == this->getMaxQ())
			return 3;
	}
};

class GridWorld
{
public:
	int i_res_, j_res_;
	CellData *q_arr2d_ = nullptr;

	GridWorld(const int& i_res, const int& j_res)
		: i_res_(i_res), j_res_(j_res)
	{
		q_arr2d_ = new CellData[i_res*j_res];
	}

	CellData& getCell(const int& i, const int& j)
	{
		return q_arr2d_[i + i_res_ * j];
	}

	bool isInside(const int& i, const int& j)
	{
		if (i < 0) return false;
		if (i >= i_res_) return false;

		if (j < 0) return false;
		if (j >= j_res_) return false;

		return true;
	}

	void printSigned(const float& v)
	{
		if (v > 0.0f) printf("+%1.1f", v);
		else if (v < 0.0f) printf("%1.1f", v);
		else
			printf(" 0.0");
	}

	void print()
	{
		for (int j = j_res_ - 1; j >= 0; j--)
		{
			for (int i = 0; i < i_res_; i++)
			{
				CellData &cell = getCell(i, j);

				printf("   "); printSigned(cell.q_[0]); printf("   "); // up
				printf("   ");
			}

			printf("\n");

			for (int i = 0; i < i_res_; i++)
			{
				CellData &cell = getCell(i, j);

				printSigned(cell.q_[2]); printf("   "); printSigned(cell.q_[3]); // left, right
				printf("  ");
			}

			printf("\n");

			for (int i = 0; i < i_res_; i++)
			{
				CellData &cell = getCell(i, j);

				printf("   "); printSigned(cell.q_[1]); printf("   "); // down
				printf("   ");
			}

			printf("\n\n");
		}
	}
};

class Agent
{
public:
	int i_, j_;
	double reward_;		

	Agent()
		: i_(0), j_(0), reward_(0.0)
	{}
};



int num = 0;
short collisionFilterGroup = short(btBroadphaseProxy::CharacterFilter);
short collisionFilterMask = short(btBroadphaseProxy::AllFilter ^ (btBroadphaseProxy::CharacterFilter));
static btScalar radius(0.2);
const int world_res_i = 6, world_res_j = 4;

GridWorld world(world_res_i, world_res_j);
Agent my_agent;
struct BasicExample : public CommonRigidBodyBase
{

	int j;
	int num_data = 0;
	int handled = 1;
	
	bool m_once;
	
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
void initState(BasicExample* target) {
	target->m_guiHelper->removeAllGraphicsInstances();
	target->initPhysics();
}

void moveLeft(btHingeConstraint *target) {
	target->setLimit(-M_PI / 1.2f, M_PI / 1.2f);
	target->enableAngularMotor(true, -45.0, 4000.f);

}

void moveRight(btHingeConstraint *target) {
	target->setLimit(-M_PI / 1.2f, M_PI / 1.2f);
	target->enableAngularMotor(true, 45.0, 4000.f);
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
	

	int i = my_agent.i_, j = my_agent.j_;
	int num = world.getCell(i, j).getMaxQ_Num();
	std::cout << num << ", " ;
	switch (num)
	{
	case 0:
		j++; // up
		moveLeft(hinge_shader);
		break;
	case 1:
		j--; // down
		moveRight(hinge_shader);
		break;
	case 2:
		moveLeft(hinge_elbow);
		i--; // left
		break;
	case 3:
		i++; // right
		moveRight(hinge_elbow);
		break;
	}
	
	if (world.isInside(i, j) == true) // move robot if available
	{
		// move agent
		my_agent.i_ = i;
		my_agent.j_ = j;
	
		// reset if agent is in final cells
		if (world.getCell(i, j).reward_ == 1 || world.getCell(i, j).reward_ == -1) {
			my_agent.i_ = 0;
			my_agent.j_ = 0;
			initState(this);
		}
	}
	
	m_dynamicsWorld->stepSimulation(1. / 240, 0);

	static int count = 0;

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
		
		btSphereShape* linkSphere_1 = new btSphereShape(radius);

		btTransform start; start.setIdentity();
		groundOrigin_target = btVector3(-0.4f, 4.0f, -1.6f);

		start.setOrigin(groundOrigin_target);
		body = createRigidBody(0, start, linkSphere_1);

		body->setFriction(0);

	}

	pos_z_ = linkBody->getCenterOfMassPosition().getZ();
	pos_y_ = linkBody->getCenterOfMassPosition().getY();
	target_z_ = body->getCenterOfMassPosition().getZ();
	target_y_ = body->getCenterOfMassPosition().getY();

	distance_ = (Vector2D<double>(pos_y_, pos_z_) - Vector2D<double>(target_y_, target_z_)).getMagnitude();


	std::cout << "Hello, grid world!" << std::endl;

	

	for (int j = 0; j < world_res_j; j++)
		for (int i = 0; i < world_res_i; i++)
		{
			world.getCell(i, j).reward_ = -0.1;
		}

	world.getCell(5, 3).reward_ = 1.0;
	world.getCell(5, 0).reward_ = -1.0;
	world.getCell(1, 1).reward_ = -1.0;
	world.getCell(2, 1).reward_ = -1.0;
	world.getCell(3, 1).reward_ = -1.0;

	world.print();

	std::cout << "----------------------------------------" << std::endl;

	for (int t = 0; t < 100000; t++)
	{
		const int action = rand() % 4; // random policy

		int i = my_agent.i_, j = my_agent.j_;
		int i_old = i, j_old = j;

		switch (action)
		{
		case 0:
			j++; // up
			break;
		case 1:
			j--; // down
			break;
		case 2:
			i--; // left
			break;
		case 3:
			i++; // right
			break;
		}

		if (world.isInside(i, j) == true) // move robot if available
		{
			my_agent.i_ = i;
			my_agent.j_ = j;
			my_agent.reward_ += world.getCell(i_old, j_old).reward_;
			switch (action)
			{
			case 0:
				world.getCell(i_old, j_old).q_[0] = world.getCell(i_old, j_old).q_[0] + 0.5*(world.getCell(i, j).reward_ + 0.9* world.getCell(i, j).getMaxQ() - world.getCell(i_old, j_old).q_[0]);
				break;
			case 1:
				world.getCell(i_old, j_old).q_[1] = world.getCell(i_old, j_old).q_[1] + 0.5*(world.getCell(i, j).reward_ + 0.9* world.getCell(i, j).getMaxQ() - world.getCell(i_old, j_old).q_[1]);
				break;
			case 2:
				world.getCell(i_old, j_old).q_[2] = world.getCell(i_old, j_old).q_[2] + 0.5*(world.getCell(i, j).reward_ + 0.9* world.getCell(i, j).getMaxQ() - world.getCell(i_old, j_old).q_[2]);
				break;
			case 3:
				world.getCell(i_old, j_old).q_[3] = world.getCell(i_old, j_old).q_[3] + 0.5*(world.getCell(i, j).reward_ + 0.9* world.getCell(i, j).getMaxQ() - world.getCell(i_old, j_old).q_[3]);
				break;
			}
			if (world.getCell(i, j).reward_ == 1 || world.getCell(i, j).reward_ == -1) {
				my_agent.i_ = 0;
				my_agent.j_ = 0;
				my_agent.reward_ = 0.0;
			}
		}
		else
		{
			world.getCell(i_old, j_old).q_[action] = -1.0;
		}
	}

	world.print();

	m_guiHelper->autogenerateGraphicsObjects(m_dynamicsWorld);
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
			//moveTarget();
			initState(this);
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
