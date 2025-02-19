#include <iostream>
#include <memory>
#include <thread>
#include <cstring>

#include "SDKMinimalClient.hpp"
#include "ManusSDKTypes.h"



// Manus Hand functionality from here down
// Initialize the static member variable
SDKMinimalClient *SDKMinimalClient::s_Instance = nullptr;

SDKMinimalClient::SDKMinimalClient()
{
	s_Instance = this;
}

SDKMinimalClient::~SDKMinimalClient()
{
	s_Instance = nullptr;
}

/// @brief Initialize the sample console and the SDK.
/// This function attempts to resize the console window and then proceeds to initialize the SDK's interface.
ClientReturnCode SDKMinimalClient::Initialize()
{
	
	/*if (!PlatformSpecificInitialization())
	{
		return ClientReturnCode::ClientReturnCode_FailedPlatformSpecificInitialization;
	}*/

        

	const ClientReturnCode t_IntializeResult = InitializeSDK();
	if (t_IntializeResult != ClientReturnCode::ClientReturnCode_Success)
	{
		ROS_ERROR("Failed to initialize the Manus SDK");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to initialize the Manus SDK");
		return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	}

	return ClientReturnCode::ClientReturnCode_Success;
}

/// @brief Initialize the sdk, register the callbacks and set the coordinate system.
/// This needs to be done before any of the other SDK functions can be used.
ClientReturnCode SDKMinimalClient::InitializeSDK()
{
	// before we can use the SDK, some internal SDK bits need to be initialized.
	// however after initializing, the SDK is not yet connected to a host or doing anything network related just yet.
	const SDKReturnCode t_InitializeResult = CoreSdk_Initialize(SessionType::SessionType_CoreSDK, true);
	if (t_InitializeResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to initialize the SDK");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to initialize the SDK");
		return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	}

	const ClientReturnCode t_CallBackResults = RegisterAllCallbacks();
	if (t_CallBackResults != ::ClientReturnCode::ClientReturnCode_Success)
	{
		ROS_ERROR("Failed to register all callbacks");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to register all callbacks");
		return t_CallBackResults;
	}

	// after everything is registered and initialized as seen above
	// we must also set the coordinate system being used for the data in this client.
	// (each client can have their own settings. unreal and unity for instance use different coordinate systems)
	// if this is not set, the SDK will not connect to any Manus core host.

	// CoordinateSystemVUH t_VUH;
	// CoordinateSystemVUH_Init(&t_VUH);
	// // t_VUH.handedness = Side::Side_Left; // this is currently set to unreal mode.
	// // t_VUH.up = AxisPolarity::AxisPolarity_PositiveY;
	// // t_VUH.view = AxisView::AxisView_ZFromViewer;
	// t_VUH.handedness = Side::Side_Right; // this is currently set to ROS mode.
	// t_VUH.up = AxisPolarity::AxisPolarity_PositiveZ;
	// t_VUH.view = AxisView::AxisView_XFromViewer;
	// t_VUH.unitScale = 1.0f; // 1.0 is meters, 0.01 is cm, 0.001 is mm.

	// const SDKReturnCode t_CoordinateResult = CoreSdk_InitializeCoordinateSystemWithVUH(t_VUH, false);

	CoordinateSystemVUH t_VUH;
	CoordinateSystemVUH_Init(&t_VUH);
	t_VUH.handedness = Side::Side_Left;
	t_VUH.up = AxisPolarity::AxisPolarity_PositiveY;
	t_VUH.view = AxisView::AxisView_ZFromViewer;
	t_VUH.unitScale = 1.0f; //1.0 is meters, 0.01 is cm, 0.001 is mm.

	// The above specified coordinate system is used to initialize and the coordinate space is specified (world/local).
	const SDKReturnCode t_CoordinateResult = CoreSdk_InitializeCoordinateSystemWithVUH(t_VUH, true);

	if (t_CoordinateResult != SDKReturnCode::SDKReturnCode_Success)
	{
		return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	}

	// set the hand motion mode of the RawSkeletonStream. This is optional and can be set to any of the HandMotion enum values. Default = None
	// auto will make it move based on available tracking data. If none is available IMU rotation will be used.
	// const SDKReturnCode t_HandMotionResult = CoreSdk_SetRawSkeletonHandMotion(HandMotion_None);
	// if (t_HandMotionResult != SDKReturnCode::SDKReturnCode_Success)
	// {
	// 	return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	// }

	return ClientReturnCode::ClientReturnCode_Success;
}

/// @brief When you are done with the SDK, don't forget to nicely shut it down
/// this will close all connections to the host, close any threads and clean up after itself
/// after this is called it is expected to exit the client program. If not it needs to call initialize again.
ClientReturnCode SDKMinimalClient::ShutDown()
{
	const SDKReturnCode t_Result = CoreSdk_ShutDown();
	if (t_Result != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to shut down the SDK");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to shut down the SDK");
		return ClientReturnCode::ClientReturnCode_FailedToShutDownSDK;
	}

	/*if (!PlatformSpecificShutdown())
	{
		return ClientReturnCode::ClientReturnCode_FailedPlatformSpecificShutdown;
	}*/
	
	ROS_INFO("Manus SDK has shut down.");
	// RCLCPP_INFO(m_PublisherNode->get_logger(), "Manus SDK has shut down.");
	return ClientReturnCode::ClientReturnCode_Success;
}

/// @brief Used to register the callbacks between sdk and core.
/// Callbacks that are registered functions that get called when a certain 'event' happens, such as data coming in from Manus Core.
/// All of these are optional, but depending on what data you require you may or may not need all of them.
ClientReturnCode SDKMinimalClient::RegisterAllCallbacks()
{
	// Register the callback for when manus core is sending Skeleton data
	// it is optional, but without it you can not see any resulting skeleton data.
	// see OnSkeletonStreamCallback for more details.
	const SDKReturnCode t_RegisterSkeletonCallbackResult = CoreSdk_RegisterCallbackForSkeletonStream(*OnSkeletonStreamCallback);
	if (t_RegisterSkeletonCallbackResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to register the skeleton callback");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to register the skeleton callback");
		return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	}

	const SDKReturnCode t_RegisterRawSkeletonCallbackResult = CoreSdk_RegisterCallbackForRawSkeletonStream(*OnRawSkeletonStreamCallback);
	if (t_RegisterRawSkeletonCallbackResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to register callback function for processing raw skeletal data from Manus Core");
		return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	}

	return ClientReturnCode::ClientReturnCode_Success;
}

/// @brief This function is called when the SDK has connected to a host.
/// This function is called when the SDK has connected to a host.  Split out from the original Run function.
void SDKMinimalClient::ConnectToHost()
{
	// first loop until we get a connection
	std::cout << "minimal client is connecting to host. (make sure it is running)\n";
	
	ROS_INFO("Manus client is connecting to host. (make sure it is running)");
	// RCLCPP_INFO(m_PublisherNode->get_logger(), "Manus client is connecting to host. (make sure it is running)");
	while (Connect() != ClientReturnCode::ClientReturnCode_Success)
	{
		ROS_WARN("Manus client could not connect. Trying again in a second.");
		// RCLCPP_WARN(m_PublisherNode->get_logger(), "Manus client could not connect. Trying again in a second.");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	// ROS_ERROR("Manus client connected to host.");
	ROS_INFO("Manus client connected to host.");

	// Upload a simple skeleton with a chain. This will just be a left hand for the first user index.
	LoadTestSkeleton();

	// set the hand motion mode of the RawSkeletonStream. This is optional and can be set to any of the HandMotion enum values. Default = None
	// auto will make it move based on available tracking data. If none is available IMU rotation will be used.
	const SDKReturnCode t_HandMotionResult = CoreSdk_SetRawSkeletonHandMotion(HandMotion_None);
	// if (t_HandMotionResult != SDKReturnCode::SDKReturnCode_Success)
	// {
	// 	return ClientReturnCode::ClientReturnCode_FailedToInitialize;
	// }
}

/// @brief Main loop that receives data from the SDK and processes it.
bool SDKMinimalClient::Run()
{
	m_HasNewSkeletonData = false;

	// Check if there is new data, otherwise, we just wait.
	m_SkeletonMutex.lock();
	if (m_NextSkeleton != nullptr)
	{
		if (m_Skeleton != nullptr)
			delete m_Skeleton;
		m_Skeleton = m_NextSkeleton;
		m_NextSkeleton = nullptr;
        m_HasNewSkeletonData = true;
	}
	m_SkeletonMutex.unlock();

	m_RawSkeletonMutex.lock();
	if (m_NextRawSkeleton != nullptr)
	{
		if (m_RawSkeleton != nullptr)
			delete m_RawSkeleton;
		m_RawSkeleton = m_NextRawSkeleton;
		m_NextRawSkeleton = nullptr;
	}
	m_RawSkeletonMutex.unlock();

    m_FrameCounter++;

    return m_HasNewSkeletonData;
}


/// @brief the client will now try to connect to manus core via the SDK.
ClientReturnCode SDKMinimalClient::Connect()
{
	SDKReturnCode t_StartResult = CoreSdk_LookForHosts(1, false);
	if (t_StartResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to find hosts");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to find hosts");
		return ClientReturnCode::ClientReturnCode_FailedToFindHosts;
	}

	uint32_t t_NumberOfHostsFound = 0;
	SDKReturnCode t_NumberResult = CoreSdk_GetNumberOfAvailableHostsFound(&t_NumberOfHostsFound);
	if (t_NumberResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to find hosts");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to find hosts");
		return ClientReturnCode::ClientReturnCode_FailedToFindHosts;
	}

	if (t_NumberOfHostsFound == 0)
	{
		ROS_ERROR("No hosts found");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "No hosts found");
		return ClientReturnCode::ClientReturnCode_FailedToFindHosts;
	}

	std::unique_ptr<ManusHost[]> t_AvailableHosts;
	t_AvailableHosts.reset(new ManusHost[t_NumberOfHostsFound]);

	SDKReturnCode t_HostsResult = CoreSdk_GetAvailableHostsFound(t_AvailableHosts.get(), t_NumberOfHostsFound);
	if (t_HostsResult != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to find hosts");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to find hosts");
		return ClientReturnCode::ClientReturnCode_FailedToFindHosts;
	}

	SDKReturnCode t_ConnectResult = CoreSdk_ConnectToHost(t_AvailableHosts[0]);

	if (t_ConnectResult == SDKReturnCode::SDKReturnCode_NotConnected)
	{
		ROS_ERROR("Failed to connect to host");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to connect to host");
		return ClientReturnCode::ClientReturnCode_FailedToConnect;
	}

	return ClientReturnCode::ClientReturnCode_Success;
}

/// @brief This function sets up a very minimalistic hand skeleton.
/// In order to have any 3d positional/rotational information from the gloves or body,
/// one needs to setup a skeleton on which this data can be applied.
/// In the case of this sample we create a Hand skeleton in order to get skeleton information
/// in the OnSkeletonStreamCallback function. This sample does not contain any 3D rendering, so
/// we will not be applying the returned data on anything.
void SDKMinimalClient::LoadTestSkeleton()
{
	for (uint32_t hand = 0; hand < 2; hand++)
    {
        uint32_t t_SklIndex = 0;

        // Create a skeleton setup for the hand

        SkeletonSetupInfo t_SKL;
        SkeletonSetupInfo_Init(&t_SKL);
        t_SKL.type = SkeletonType::SkeletonType_Hand;
        t_SKL.settings.scaleToTarget = true;
        t_SKL.settings.targetType = SkeletonTargetType::SkeletonTargetType_UserIndexData;

        // The user index is the index of the user that the skeleton is attached to.
        // If the glove does not exist then the added skeleton will not be animated.
        // Same goes for any other skeleton made for invalid users/gloves.
        t_SKL.settings.skeletonTargetUserIndexData.userIndex = 0; // Just take the first index. make sure this matches in the landscape.

		strncpy(t_SKL.name, (hand == 0) ? "RightHand" : "LeftHand", sizeof(t_SKL.name));
        
        SDKReturnCode t_Res = CoreSdk_CreateSkeletonSetup(t_SKL, &t_SklIndex);
        if (t_Res != SDKReturnCode::SDKReturnCode_Success)
        {
			ROS_ERROR("Failed to create skeleton setup");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to create skeleton setup");
            return;
        }

        // setup nodes and chains for the skeleton hand
        if (!SetupHandNodes(t_SklIndex, (hand == 0)))
        {
			ROS_ERROR("Failed to setup hand nodes");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to setup hand nodes");
            return;
        }
        if (!SetupHandChains(t_SklIndex, (hand == 0)))
        {
			ROS_ERROR("Failed to setup hand chains");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to setup hand chains");
            return;
        }

        // load skeleton
        t_Res = CoreSdk_LoadSkeleton(t_SklIndex, &m_GloveIDs[hand]);
        if (t_Res != SDKReturnCode::SDKReturnCode_Success)
        {
			ROS_ERROR("Failed to load skeleton");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to load skeleton");
            return;
        }
        else 
        {
			ROS_INFO("Skeleton ID: # loaded successfully");
			// ROS_INFO("Skeleton ID:" << &m_GloveIDs[hand] << " loaded successfully");
			// RCLCPP_INFO_STREAM(m_PublisherNode->get_logger(), "Skeleton ID:" << &m_GloveIDs[hand] << " loaded successfully");
        }
    }
}

/// @brief Skeletons are pretty extensive in their data setup
/// so we have several support functions so we can correctly receive and parse the data,
/// this function helps setup the data.
/// @param p_Id the id of the created node setup
/// @param p_ParentId the id of the node parent
/// @param p_PosX X position of the node, this is defined with respect to the global coordinate system or the local one depending on
/// the parameter p_UseWorldCoordinates set when initializing the sdk,
/// @param p_PosY Y position of the node this is defined with respect to the global coordinate system or the local one depending on
/// the parameter p_UseWorldCoordinates set when initializing the sdk,
/// @param p_PosZ Z position of the node this is defined with respect to the global coordinate system or the local one depending on
/// the parameter p_UseWorldCoordinates set when initializing the sdk,
/// @param p_Name the name of the node setup
/// @return the generated node setup
NodeSetup SDKMinimalClient::CreateNodeSetup(uint32_t p_Id, uint32_t p_ParentId, float p_PosX, float p_PosY, float p_PosZ, std::string p_Name)
{
	NodeSetup t_Node;
	NodeSetup_Init(&t_Node);
	t_Node.id = p_Id; // Every ID needs to be unique per node in a skeleton.
	strncpy(t_Node.name, p_Name.c_str(), sizeof(t_Node.name));
	t_Node.type = NodeType::NodeType_Joint;
	// Every node should have a parent unless it is the Root node.
	t_Node.parentID = p_ParentId; // Setting the node ID to its own ID ensures it has no parent.
	t_Node.settings.usedSettings = NodeSettingsFlag::NodeSettingsFlag_None;

	t_Node.transform.position.x = p_PosX;
	t_Node.transform.position.y = p_PosY;
	t_Node.transform.position.z = p_PosZ;
	return t_Node;
}

ManusVec3 SDKMinimalClient::CreateManusVec3(float p_X, float p_Y, float p_Z)
{
	ManusVec3 t_Vec;
	t_Vec.x = p_X;
	t_Vec.y = p_Y;
	t_Vec.z = p_Z;
	return t_Vec;
}

/// @brief This support function sets up the nodes for the skeleton hand
/// In order to have any 3d positional/rotational information from the gloves or body,
/// one needs to setup the skeleton on which this data should be applied.
/// In the case of this sample we create a Hand skeleton for which we want to get the calculated result.
/// The ID's for the nodes set here are the same IDs which are used in the OnSkeletonStreamCallback,
/// this allows us to create the link between Manus Core's data and the data we enter here.
bool SDKMinimalClient::SetupHandNodes(uint32_t p_SklIndex, bool isRightHand)
{
	// Define number of fingers per hand and number of joints per finger
	const uint32_t t_NumFingers = 5;
	const uint32_t t_NumJoints = 4;

	// Create an array with the initial position of each hand node.
	ManusVec3 t_Fingers_Right[t_NumFingers * t_NumJoints] = {
		CreateManusVec3(0.025320f, 0.024950f, 0.000000f), // Thumb CMC joint
		CreateManusVec3(0.032742f, 0.000000f, 0.000000f), // Thumb MCP joint
		CreateManusVec3(0.028739f, 0.000000f, 0.000000f), // Thumb IP joint
		CreateManusVec3(0.028739f, 0.000000f, 0.000000f), // Thumb Tip joint

		CreateManusVec3(0.052904f, -0.011181f, 0.000000f), // Index MCP joint
		CreateManusVec3(0.038257f, 0.000000f, 0.000000f),  // Index PIP joint
		CreateManusVec3(0.020884f, 0.000000f, 0.000000f),  // Index DIP joint
		CreateManusVec3(0.018759f, 0.000000f, 0.000000f),  // Index Tip joint

		CreateManusVec3(0.051287f, 0.000000f, 0.000000f), // Middle MCP joint
		CreateManusVec3(0.041861f, 0.000000f, 0.000000f), // Middle PIP joint
		CreateManusVec3(0.024766f, 0.000000f, 0.000000f), // Middle DIP joint
		CreateManusVec3(0.019683f, 0.000000f, 0.000000f), // Middle Tip joint

		CreateManusVec3(0.049802f, -0.011274f, 0.000000f), // Ring MCP joint
		CreateManusVec3(0.039736f, 0.000000f, 0.000000f),  // Ring PIP joint
		CreateManusVec3(0.023564f, 0.000000f, 0.000000f),  // Ring DIP joint
		CreateManusVec3(0.019868f, 0.000000f, 0.000000f),  // Ring Tip joint

		CreateManusVec3(0.047309f, -0.020145f, 0.000000f), // Pinky MCP joint
		CreateManusVec3(0.033175f, 0.000000f, 0.000000f),  // Pinky PIP joint
		CreateManusVec3(0.018020f, 0.000000f, 0.000000f),  // Pinky DIP joint
		CreateManusVec3(0.019129f, 0.000000f, 0.000000f)   // Pinky Tip joint
	};

	ManusVec3 t_Fingers_Left[t_NumFingers * t_NumJoints] = {
		CreateManusVec3(-0.025320f, 0.024950f, 0.000000f), // Thumb CMC joint
		CreateManusVec3(-0.032742f, 0.000000f, 0.000000f), // Thumb MCP joint
		CreateManusVec3(-0.028739f, 0.000000f, 0.000000f), // Thumb IP joint
		CreateManusVec3(-0.028739f, 0.000000f, 0.000000f), // Thumb Tip joint

		CreateManusVec3(-0.052904f, -0.011181f, 0.000000f), // Index MCP joint
		CreateManusVec3(-0.038257f, 0.000000f, 0.000000f),	// Index PIP joint
		CreateManusVec3(-0.020884f, 0.000000f, 0.000000f),	// Index DIP joint
		CreateManusVec3(-0.018759f, 0.000000f, 0.000000f),	// Index Tip joint

		CreateManusVec3(-0.051287f, 0.000000f, 0.000000f), // Middle MCP joint
		CreateManusVec3(-0.041861f, 0.000000f, 0.000000f), // Middle PIP joint
		CreateManusVec3(-0.024766f, 0.000000f, 0.000000f), // Middle DIP joint
		CreateManusVec3(-0.019683f, 0.000000f, 0.000000f), // Middle Tip joint

		CreateManusVec3(-0.049802f, 0.011274f, 0.000000f), // Ring MCP joint
		CreateManusVec3(-0.039736f, 0.000000f, 0.000000f), // Ring PIP joint
		CreateManusVec3(-0.023564f, 0.000000f, 0.000000f), // Ring DIP joint
		CreateManusVec3(-0.019868f, 0.000000f, 0.000000f), // Ring Tip joint

		CreateManusVec3(-0.047309f, 0.020145f, 0.000000f), // Pinky MCP joint
		CreateManusVec3(-0.033175f, 0.000000f, 0.000000f), // Pinky PIP joint
		CreateManusVec3(-0.018020f, 0.000000f, 0.000000f), // Pinky DIP joint
		CreateManusVec3(-0.019129f, 0.000000f, 0.000000f)  // Pinky Tip joint
	};

	// skeleton entry is already done. just the nodes now.
	// setup a very simple node hierarchy for fingers
	// first setup the root node
	//
	// root, This node has ID 0 and parent ID 0, to indicate it has no parent.
	SDKReturnCode t_Res = CoreSdk_AddNodeToSkeletonSetup(p_SklIndex, CreateNodeSetup(0, 0, 0, 0, 0, "Hand"));
	if (t_Res != SDKReturnCode::SDKReturnCode_Success)
	{
		ROS_ERROR("Failed to Add Node To Skeleton Setup");
		// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to Add Node To Skeleton Setup");
		return false;
	}

	// then loop for 5 fingers
	int t_FingerId = 0;
	for (uint32_t i = 0; i < t_NumFingers; i++)
	{
		uint32_t t_ParentID = 0;
		// then the digits of the finger that are linked to the root of the finger.
		for (uint32_t j = 0; j < t_NumJoints; j++)
		{
			// Setup the handeness of the Manus Glove
			if (isRightHand) // Right Hand
				t_Res = CoreSdk_AddNodeToSkeletonSetup(p_SklIndex, CreateNodeSetup(1 + t_FingerId + j, t_ParentID, t_Fingers_Right[i * 4 + j].x, t_Fingers_Right[i * 4 + j].y, t_Fingers_Right[i * 4 + j].z, "fingerdigit"));
			else // Left Hand
				t_Res = CoreSdk_AddNodeToSkeletonSetup(p_SklIndex, CreateNodeSetup(1 + t_FingerId + j, t_ParentID, t_Fingers_Left[i * 4 + j].x, t_Fingers_Left[i * 4 + j].y, t_Fingers_Left[i * 4 + j].z, "fingerdigit"));
			if (t_Res != SDKReturnCode::SDKReturnCode_Success)
			{
				ROS_ERROR("Failed to Add Node To Skeleton Setup");
				// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to Add Node To Skeleton Setup");
				return false;
			}
			t_ParentID = 1 + t_FingerId + j;
		}
		t_FingerId += t_NumJoints;
	}
	return true;
}

/// @brief This function sets up some basic hand chains.
/// Chains are required for a Skeleton to be able to be animated, it basically tells Manus Core
/// which nodes belong to which body part and what data needs to be applied to which node.
/// @param p_SklIndex The index of the temporary skeleton on which the chains will be added.
/// @return Returns true if everything went fine, otherwise returns false.
bool SDKMinimalClient::SetupHandChains(uint32_t p_SklIndex, bool isRightHand)
{
	// Add the Hand chain, this identifies the wrist of the hand
	{
		ChainSettings t_ChainSettings;
		ChainSettings_Init(&t_ChainSettings);
		t_ChainSettings.usedSettings = ChainType::ChainType_Hand;
		t_ChainSettings.hand.handMotion = HandMotion::HandMotion_IMU;
		t_ChainSettings.hand.fingerChainIdsUsed = 5; // We have 5 fingers
		t_ChainSettings.hand.fingerChainIds[0] = 1;	 // Links to the other chains we will define further down
		t_ChainSettings.hand.fingerChainIds[1] = 2;
		t_ChainSettings.hand.fingerChainIds[2] = 3;
		t_ChainSettings.hand.fingerChainIds[3] = 4;
		t_ChainSettings.hand.fingerChainIds[4] = 5;

		ChainSetup t_Chain;
		ChainSetup_Init(&t_Chain);
		t_Chain.id = 0; // Every ID needs to be unique per chain in a skeleton.
		t_Chain.type = ChainType::ChainType_Hand;
		t_Chain.dataType = ChainType::ChainType_Hand;
		t_Chain.side = isRightHand ? Side::Side_Right : Side::Side_Left; // Set the proper hand side
		t_Chain.dataIndex = 0;
		t_Chain.nodeIdCount = 1;
		t_Chain.nodeIds[0] = 0; // This links to the hand node created in the SetupHandNodes
		t_Chain.settings = t_ChainSettings;

		SDKReturnCode t_Res = CoreSdk_AddChainToSkeletonSetup(p_SklIndex, t_Chain);
		if (t_Res != SDKReturnCode::SDKReturnCode_Success)
		{
			ROS_ERROR("Failed to Add Chain To Skeleton Setup");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to Add Chain To Skeleton Setup");
			return false;
		}
	}

	// Add the 5 finger chains
	const ChainType t_FingerTypes[5] = {ChainType::ChainType_FingerThumb,
										ChainType::ChainType_FingerIndex,
										ChainType::ChainType_FingerMiddle,
										ChainType::ChainType_FingerRing,
										ChainType::ChainType_FingerPinky};
	for (int i = 0; i < 5; i++)
	{
		ChainSettings t_ChainSettings;
		ChainSettings_Init(&t_ChainSettings);
		t_ChainSettings.usedSettings = t_FingerTypes[i];
		t_ChainSettings.finger.handChainId = 0; // This links to the wrist chain above.

		// This identifies the metacarpal bone, if none exists, or the chain is a thumb it should be set to -1.
		// The metacarpal bone should not be part of the finger chain, unless you are defining a thumb which does need it.
		t_ChainSettings.finger.metacarpalBoneId = -1;
		t_ChainSettings.finger.useLeafAtEnd = false; // This is set to true if there is a leaf bone to the tip of the finger.
		ChainSetup t_Chain;
		ChainSetup_Init(&t_Chain);
		t_Chain.id = i + 1; // Every ID needs to be unique per chain in a skeleton.
		t_Chain.type = t_FingerTypes[i];
		t_Chain.dataType = t_FingerTypes[i];
		t_Chain.side = isRightHand ? Side::Side_Right : Side::Side_Left; // Set the proper hand side
		t_Chain.dataIndex = 0;
		if (i == 0) // Thumb
		{
            
			t_Chain.nodeIdCount = 4; // The amount of node id's used in the array
			t_Chain.nodeIds[0] = 1;	 // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[1] = 2;	 // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[2] = 3;	 // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[3] = 4;	 // this links to the hand node created in the SetupHandNodes
		}
		else // All other fingers
		{
			t_Chain.nodeIdCount = 4;		  // The amount of node id's used in the array
			t_Chain.nodeIds[0] = (i * 4) + 1; // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[1] = (i * 4) + 2; // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[2] = (i * 4) + 3; // this links to the hand node created in the SetupHandNodes
			t_Chain.nodeIds[3] = (i * 4) + 4; // this links to the hand node created in the SetupHandNodes
		}
		t_Chain.settings = t_ChainSettings;

		SDKReturnCode t_Res = CoreSdk_AddChainToSkeletonSetup(p_SklIndex, t_Chain);
		if (t_Res != SDKReturnCode::SDKReturnCode_Success)
		{
			ROS_ERROR("Failed to Add Chain To Skeleton Setup");
			// RCLCPP_ERROR(m_PublisherNode->get_logger(), "Failed to Add Chain To Skeleton Setup");
			return false;
		}
	}
	return true;
}

/// @brief This gets called when the client is connected to manus core
/// @param p_SkeletonStreamInfo contains the meta data on how much data regarding the skeleton we need to get from the SDK.
void SDKMinimalClient::OnSkeletonStreamCallback(const SkeletonStreamInfo *const p_SkeletonStreamInfo)
{
	if (s_Instance)
	{
		ClientSkeletonCollection *t_NxtClientSkeleton = new ClientSkeletonCollection();
		t_NxtClientSkeleton->skeletons.resize(p_SkeletonStreamInfo->skeletonsCount);

		for (uint32_t i = 0; i < p_SkeletonStreamInfo->skeletonsCount; i++)
		{
			CoreSdk_GetSkeletonInfo(i, &t_NxtClientSkeleton->skeletons[i].info);
			t_NxtClientSkeleton->skeletons[i].nodes = new SkeletonNode[t_NxtClientSkeleton->skeletons[i].info.nodesCount];
			CoreSdk_GetSkeletonData(i, t_NxtClientSkeleton->skeletons[i].nodes, t_NxtClientSkeleton->skeletons[i].info.nodesCount);
		}
		s_Instance->m_SkeletonMutex.lock();
		if (s_Instance->m_NextSkeleton != nullptr)
			delete s_Instance->m_NextSkeleton;
		s_Instance->m_NextSkeleton = t_NxtClientSkeleton;
		s_Instance->m_SkeletonMutex.unlock();
	}
}
void SDKMinimalClient::OnRawSkeletonStreamCallback(const SkeletonStreamInfo* const p_RawSkeletonStreamInfo)
{
	if (s_Instance)
	{
		ClientRawSkeletonCollection* t_NxtClientRawSkeleton = new ClientRawSkeletonCollection();
		t_NxtClientRawSkeleton->skeletons.resize(p_RawSkeletonStreamInfo->skeletonsCount);

		for (uint32_t i = 0; i < p_RawSkeletonStreamInfo->skeletonsCount; i++)
		{
			//Retrieves info on the skeletonData, like deviceID and the amount of nodes.
			CoreSdk_GetRawSkeletonInfo(i, &t_NxtClientRawSkeleton->skeletons[i].info);
			t_NxtClientRawSkeleton->skeletons[i].nodes.resize(t_NxtClientRawSkeleton->skeletons[i].info.nodesCount);
			t_NxtClientRawSkeleton->skeletons[i].info.publishTime = p_RawSkeletonStreamInfo->publishTime;

			//Retrieves the skeletonData, which contains the node data.
			CoreSdk_GetRawSkeletonData(i, t_NxtClientRawSkeleton->skeletons[i].nodes.data(), t_NxtClientRawSkeleton->skeletons[i].info.nodesCount);
		}
		s_Instance->m_RawSkeletonMutex.lock();
		if (s_Instance->m_NextRawSkeleton != nullptr) delete s_Instance->m_NextRawSkeleton;
		s_Instance->m_NextRawSkeleton = t_NxtClientRawSkeleton;
		s_Instance->m_RawSkeletonMutex.unlock();
	}
}