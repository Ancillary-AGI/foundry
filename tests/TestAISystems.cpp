#include "gtest/gtest.h"
#include "GameEngine/ai/BehaviorTree.h"
#include "GameEngine/ai/DecisionTree.h"
#include "GameEngine/ai/FiniteStateMachine.h"
#include "GameEngine/ai/NavigationSystem.h"
#include "GameEngine/ai/ReinforcementLearning.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for AI Systems tests
 */
class AISystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test behavior tree system
 */
TEST_F(AISystemsTest, BehaviorTree) {
    BehaviorTree bt;

    // Test behavior tree initialization
    EXPECT_TRUE(bt.initialize());
    EXPECT_TRUE(bt.isInitialized());

    // Test node creation
    BTNodeID rootNode = bt.createNode(BTNodeType::Selector, "Root");
    BTNodeID child1 = bt.createNode(BTNodeType::Sequence, "Child1");
    BTNodeID child2 = bt.createNode(BTNodeType::Action, "Child2");

    EXPECT_GT(rootNode, 0);
    EXPECT_GT(child1, 0);
    EXPECT_GT(child2, 0);

    // Test tree structure
    bt.addChild(rootNode, child1);
    bt.addChild(rootNode, child2);

    EXPECT_EQ(bt.getChildCount(rootNode), 2);
    EXPECT_TRUE(bt.hasChildren(rootNode));

    // Test node properties
    bt.setNodePrecondition(child1, "HasTarget");
    EXPECT_EQ(bt.getNodePrecondition(child1), "HasTarget");

    bt.setNodeAction(child2, "Attack");
    EXPECT_EQ(bt.getNodeAction(child2), "Attack");

    // Test tree execution
    bt.setBlackboardValue("HasTarget", true);
    bt.setBlackboardValue("CanAttack", true);

    BTStatus status = bt.tick(rootNode);
    EXPECT_NE(status, BTStatus::Invalid);

    // Test tree validation
    EXPECT_TRUE(bt.validateTree(rootNode));

    // Test cleanup
    bt.destroyNode(child2);
    bt.destroyNode(child1);
    bt.destroyNode(rootNode);

    bt.shutdown();
    EXPECT_FALSE(bt.isInitialized());
}

/**
 * @brief Test decision tree system
 */
TEST_F(AISystemsTest, DecisionTree) {
    DecisionTree dt;

    // Test decision tree initialization
    EXPECT_TRUE(dt.initialize());
    EXPECT_TRUE(dt.isInitialized());

    // Test decision node creation
    DTNodeID rootNode = dt.createDecisionNode("IsEnemyNear", "Distance < 10");
    DTNodeID trueNode = dt.createActionNode("Attack");
    DTNodeID falseNode = dt.createActionNode("Patrol");

    EXPECT_GT(rootNode, 0);
    EXPECT_GT(trueNode, 0);
    EXPECT_GT(falseNode, 0);

    // Test tree structure
    dt.setTrueChild(rootNode, trueNode);
    dt.setFalseChild(rootNode, falseNode);

    EXPECT_EQ(dt.getTrueChild(rootNode), trueNode);
    EXPECT_EQ(dt.getFalseChild(rootNode), falseNode);

    // Test decision evaluation
    dt.setVariable("Distance", 5.0f); // Enemy is near
    std::string action1 = dt.evaluate(rootNode);
    EXPECT_EQ(action1, "Attack");

    dt.setVariable("Distance", 15.0f); // Enemy is far
    std::string action2 = dt.evaluate(rootNode);
    EXPECT_EQ(action2, "Patrol");

    // Test tree traversal
    std::vector<std::string> path = dt.getDecisionPath(rootNode);
    EXPECT_GT(path.size(), 0);

    // Test cleanup
    dt.destroyNode(falseNode);
    dt.destroyNode(trueNode);
    dt.destroyNode(rootNode);

    dt.shutdown();
    EXPECT_FALSE(dt.isInitialized());
}

/**
 * @brief Test finite state machine
 */
TEST_F(AISystemsTest, FiniteStateMachine) {
    FiniteStateMachine fsm;

    // Test FSM initialization
    EXPECT_TRUE(fsm.initialize());
    EXPECT_TRUE(fsm.isInitialized());

    // Test state creation
    StateID idleState = fsm.createState("Idle");
    StateID chaseState = fsm.createState("Chase");
    StateID attackState = fsm.createState("Attack");

    EXPECT_GT(idleState, 0);
    EXPECT_GT(chaseState, 0);
    EXPECT_GT(attackState, 0);

    // Test state transitions
    fsm.addTransition(idleState, chaseState, "EnemySpotted");
    fsm.addTransition(chaseState, attackState, "InRange");
    fsm.addTransition(attackState, idleState, "EnemyDefeated");

    EXPECT_TRUE(fsm.hasTransition(idleState, chaseState));
    EXPECT_TRUE(fsm.hasTransition(chaseState, attackState));
    EXPECT_TRUE(fsm.hasTransition(attackState, idleState));

    // Test state machine execution
    fsm.setCurrentState(idleState);
    EXPECT_EQ(fsm.getCurrentState(), idleState);

    // Trigger transition
    fsm.triggerEvent("EnemySpotted");
    EXPECT_EQ(fsm.getCurrentState(), chaseState);

    fsm.triggerEvent("InRange");
    EXPECT_EQ(fsm.getCurrentState(), attackState);

    fsm.triggerEvent("EnemyDefeated");
    EXPECT_EQ(fsm.getCurrentState(), idleState);

    // Test state actions
    fsm.setStateAction(idleState, "LookForEnemies");
    fsm.setStateAction(chaseState, "MoveTowardsEnemy");
    fsm.setStateAction(attackState, "FireWeapon");

    EXPECT_EQ(fsm.getStateAction(idleState), "LookForEnemies");
    EXPECT_EQ(fsm.getStateAction(chaseState), "MoveTowardsEnemy");
    EXPECT_EQ(fsm.getStateAction(attackState), "FireWeapon");

    // Test state machine update
    fsm.update(0.016f);
    // Should execute current state action

    // Test cleanup
    fsm.destroyState(attackState);
    fsm.destroyState(chaseState);
    fsm.destroyState(idleState);

    fsm.shutdown();
    EXPECT_FALSE(fsm.isInitialized());
}

/**
 * @brief Test navigation system
 */
TEST_F(AISystemsTest, NavigationSystem) {
    NavigationSystem nav;

    // Test navigation initialization
    EXPECT_TRUE(nav.initialize());
    EXPECT_TRUE(nav.isInitialized());

    // Test navigation mesh creation
    NavMeshID navMesh = nav.createNavMesh();
    EXPECT_GT(navMesh, 0);

    // Test waypoint management
    WaypointID wp1 = nav.addWaypoint(Vector3(0.0f, 0.0f, 0.0f));
    WaypointID wp2 = nav.addWaypoint(Vector3(10.0f, 0.0f, 0.0f));
    WaypointID wp3 = nav.addWaypoint(Vector3(20.0f, 0.0f, 10.0f));

    EXPECT_GT(wp1, 0);
    EXPECT_GT(wp2, 0);
    EXPECT_GT(wp3, 0);

    // Test pathfinding
    std::vector<Vector3> path = nav.findPath(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f, 0.0f, 10.0f));
    EXPECT_GT(path.size(), 0);

    // Test path simplification
    std::vector<Vector3> simplifiedPath = nav.simplifyPath(path);
    EXPECT_LE(simplifiedPath.size(), path.size());

    // Test navigation queries
    Vector3 nearestPoint = nav.getNearestNavigablePoint(Vector3(5.0f, 5.0f, 5.0f));
    EXPECT_GE(nearestPoint.x, 0.0f);

    bool isNavigable = nav.isPointNavigable(Vector3(5.0f, 0.0f, 5.0f));
    // Should be navigable if on nav mesh

    // Test dynamic obstacles
    nav.addDynamicObstacle(Vector3(15.0f, 0.0f, 5.0f), 2.0f);
    EXPECT_GT(nav.getDynamicObstacleCount(), 0);

    nav.removeDynamicObstacle(Vector3(15.0f, 0.0f, 5.0f));
    // Obstacle should be removed

    // Test navigation mesh optimization
    nav.optimizeNavMesh(navMesh);
    EXPECT_TRUE(nav.isNavMeshOptimized(navMesh));

    // Test cleanup
    nav.removeWaypoint(wp3);
    nav.removeWaypoint(wp2);
    nav.removeWaypoint(wp1);
    nav.destroyNavMesh(navMesh);

    nav.shutdown();
    EXPECT_FALSE(nav.isInitialized());
}

/**
 * @brief Test reinforcement learning system
 */
TEST_F(AISystemsTest, ReinforcementLearning) {
    ReinforcementLearning rl;

    // Test RL initialization
    EXPECT_TRUE(rl.initialize());
    EXPECT_TRUE(rl.isInitialized());

    // Test environment setup
    rl.setStateSize(10);
    rl.setActionSize(4);

    EXPECT_EQ(rl.getStateSize(), 10);
    EXPECT_EQ(rl.getActionSize(), 4);

    // Test neural network creation
    rl.createNetwork(64, 32); // 64 hidden neurons, 32 output neurons
    EXPECT_TRUE(rl.hasNetwork());

    // Test training parameters
    rl.setLearningRate(0.001f);
    EXPECT_FLOAT_EQ(rl.getLearningRate(), 0.001f);

    rl.setDiscountFactor(0.99f);
    EXPECT_FLOAT_EQ(rl.getDiscountFactor(), 0.99f);

    rl.setExplorationRate(0.1f);
    EXPECT_FLOAT_EQ(rl.getExplorationRate(), 0.1f);

    // Test experience replay
    rl.setReplayBufferSize(10000);
    EXPECT_EQ(rl.getReplayBufferSize(), 10000);

    rl.enableExperienceReplay(true);
    EXPECT_TRUE(rl.isExperienceReplayEnabled());

    // Test training
    std::vector<float> state = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    int action = rl.selectAction(state);
    EXPECT_GE(action, 0);
    EXPECT_LT(action, 4);

    float reward = 1.0f;
    std::vector<float> nextState = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    rl.storeExperience(state, action, reward, nextState, false);
    EXPECT_GT(rl.getExperienceCount(), 0);

    // Test training step
    rl.trainStep();
    // Network should be updated

    // Test model saving/loading
    rl.saveModel("test_model.bin");
    rl.loadModel("test_model.bin");

    // Test cleanup
    rl.shutdown();
    EXPECT_FALSE(rl.isInitialized());
}

/**
 * @brief Test AI performance
 */
TEST_F(AISystemsTest, Performance) {
    const int numIterations = 100;

    // Measure AI operations performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        // Test behavior tree performance
        BehaviorTree bt;
        bt.initialize();

        BTNodeID root = bt.createNode(BTNodeType::Selector, "Root");
        BTNodeID child = bt.createNode(BTNodeType::Action, "Action");
        bt.addChild(root, child);

        bt.setBlackboardValue("Test", true);
        bt.tick(root);

        bt.destroyNode(child);
        bt.destroyNode(root);
        bt.shutdown();

        // Test FSM performance
        FiniteStateMachine fsm;
        fsm.initialize();

        StateID state1 = fsm.createState("State1");
        StateID state2 = fsm.createState("State2");
        fsm.addTransition(state1, state2, "Transition");
        fsm.setCurrentState(state1);
        fsm.triggerEvent("Transition");
        fsm.update(0.016f);

        fsm.destroyState(state2);
        fsm.destroyState(state1);
        fsm.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " AI operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 100 operations)
    EXPECT_LT(duration.count(), 100000);
}

/**
 * @brief Test AI memory management
 */
TEST_F(AISystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple AI systems to test memory usage
    std::vector<std::unique_ptr<BehaviorTree>> behaviorTrees;
    std::vector<std::unique_ptr<FiniteStateMachine>> stateMachines;

    for (int i = 0; i < 50; ++i) {
        auto bt = std::make_unique<BehaviorTree>();
        bt->initialize();

        BTNodeID root = bt->createNode(BTNodeType::Selector, "Root" + std::to_string(i));
        BTNodeID child = bt->createNode(BTNodeType::Action, "Action" + std::to_string(i));
        bt->addChild(root, child);

        behaviorTrees.push_back(std::move(bt));

        auto fsm = std::make_unique<FiniteStateMachine>();
        fsm->initialize();

        StateID state = fsm->createState("State" + std::to_string(i));
        fsm->setCurrentState(state);

        stateMachines.push_back(std::move(fsm));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    behaviorTrees.clear();
    stateMachines.clear();
}

/**
 * @brief Test AI error handling
 */
TEST_F(AISystemsTest, ErrorHandling) {
    BehaviorTree bt;

    // Test invalid operations
    EXPECT_NO_THROW(bt.tick(99999)); // Invalid node ID should handle gracefully
    EXPECT_NO_THROW(bt.addChild(99999, 88888)); // Invalid parent/child should handle gracefully

    // Test uninitialized operations
    EXPECT_FALSE(bt.isInitialized());
    EXPECT_NO_THROW(bt.shutdown()); // Should handle multiple shutdowns

    // Test FSM error handling
    FiniteStateMachine fsm;
    EXPECT_NO_THROW(fsm.triggerEvent("InvalidEvent")); // Invalid event should handle gracefully
    EXPECT_NO_THROW(fsm.setCurrentState(99999)); // Invalid state should handle gracefully

    // Test navigation error handling
    NavigationSystem nav;
    EXPECT_NO_THROW(nav.findPath(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f))); // Same start/end should handle gracefully
}

/**
 * @brief Test AI concurrent operations
 */
TEST_F(AISystemsTest, ConcurrentOperations) {
    const int numThreads = 4;
    const int operationsPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing AI operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            BehaviorTree bt;
            bt.initialize();

            for (int i = 0; i < operationsPerThread; ++i) {
                BTNodeID root = bt.createNode(BTNodeType::Selector, "Root" + std::to_string(t) + "_" + std::to_string(i));
                if (root > 0) {
                    bt.setBlackboardValue("Thread" + std::to_string(t), true);
                    BTStatus status = bt.tick(root);

                    bt.destroyNode(root);

                    if (status != BTStatus::Invalid) {
                        successCount++;
                    }
                }
            }

            bt.shutdown();
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test AI integration scenarios
 */
TEST_F(AISystemsTest, IntegrationScenarios) {
    // Test AI systems working together

    // Create behavior tree for enemy AI
    BehaviorTree enemyBT;
    enemyBT.initialize();

    BTNodeID root = enemyBT.createNode(BTNodeType::Selector, "EnemyRoot");
    BTNodeID patrolSequence = enemyBT.createNode(BTNodeType::Sequence, "PatrolSequence");
    BTNodeID chaseAction = enemyBT.createNode(BTNodeType::Action, "ChaseAction");

    enemyBT.addChild(root, patrolSequence);
    enemyBT.addChild(root, chaseAction);

    // Create FSM for enemy states
    FiniteStateMachine enemyFSM;
    enemyFSM.initialize();

    StateID patrolState = enemyFSM.createState("Patrol");
    StateID chaseState = enemyFSM.createState("Chase");
    StateID attackState = enemyFSM.createState("Attack");

    enemyFSM.addTransition(patrolState, chaseState, "PlayerSpotted");
    enemyFSM.addTransition(chaseState, attackState, "PlayerInRange");
    enemyFSM.addTransition(attackState, patrolState, "PlayerLost");

    enemyFSM.setCurrentState(patrolState);

    // Test coordinated AI behavior
    enemyBT.setBlackboardValue("PlayerDistance", 15.0f);
    enemyBT.setBlackboardValue("PlayerVisible", false);

    BTStatus btStatus = enemyBT.tick(root);
    EXPECT_NE(btStatus, BTStatus::Invalid);

    // FSM should still be in patrol state
    EXPECT_EQ(enemyFSM.getCurrentState(), patrolState);

    // Simulate player spotted
    enemyBT.setBlackboardValue("PlayerDistance", 5.0f);
    enemyBT.setBlackboardValue("PlayerVisible", true);

    btStatus = enemyBT.tick(root);
    enemyFSM.triggerEvent("PlayerSpotted");

    // Should transition to chase state
    EXPECT_EQ(enemyFSM.getCurrentState(), chaseState);

    // Test navigation integration
    NavigationSystem nav;
    nav.initialize();

    std::vector<Vector3> path = nav.findPath(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f));
    EXPECT_GT(path.size(), 0);

    // Clean up
    nav.shutdown();
    enemyFSM.destroyState(attackState);
    enemyFSM.destroyState(chaseState);
    enemyFSM.destroyState(patrolState);
    enemyFSM.shutdown();

    enemyBT.destroyNode(chaseAction);
    enemyBT.destroyNode(patrolSequence);
    enemyBT.destroyNode(root);
    enemyBT.shutdown();
}

/**
 * @brief Test AI learning and adaptation
 */
TEST_F(AISystemsTest, LearningAndAdaptation) {
    ReinforcementLearning rl;
    rl.initialize();

    rl.setStateSize(4);
    rl.setActionSize(2);
    rl.createNetwork(32, 16);

    // Test Q-learning update
    std::vector<float> state = {1.0f, 0.0f, 0.0f, 0.0f};
    int action = rl.selectAction(state);

    // Simulate reward
    float reward = 1.0f;
    std::vector<float> nextState = {0.0f, 1.0f, 0.0f, 0.0f};

    rl.storeExperience(state, action, reward, nextState, false);

    // Train the network
    for (int i = 0; i < 10; ++i) {
        rl.trainStep();
    }

    // Test policy improvement
    int newAction = rl.selectAction(state);
    // Action selection should be consistent after training

    // Test model persistence
    rl.saveModel("ai_model.bin");
    rl.loadModel("ai_model.bin");

    // Model should still work after loading
    int loadedAction = rl.selectAction(state);
    EXPECT_GE(loadedAction, 0);
    EXPECT_LT(loadedAction, 2);

    rl.shutdown();
}

} // namespace Tests
} // namespace FoundryEngine
