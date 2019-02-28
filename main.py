# -*- coding: utf-8 -*-

import tensorflow as tf
import numpy as np
import random
import time
import sys

from model import DQN

from server import Server
from fileImport import FileLoad

tf.app.flags.DEFINE_boolean("train", False, "학습모드. 게임을 화면에 보여주지 않습니다.")
FLAGS = tf.app.flags.FLAGS

# 최대 학습 횟수
MAX_EPISODE = 10000 * 10
# 1000번의 학습마다 한 번씩 타겟 네트웍을 업데이트합니다.
TARGET_UPDATE_INTERVAL = 1000
# 4 프레임마다 한 번씩 학습합니다.
TRAIN_INTERVAL = 4
# 학습 데이터를 어느정도 쌓은 후, 일정 시간 이후에 학습을 시작하도록 합니다.
OBSERVE = 100

# action: 100
NUM_ACTION = 6
SCREEN_WIDTH = 5
SCREEN_HEIGHT = 12+1
server = Server('', 50008)

def reshapeFromPacket(state):
    arr = []
    for i in range(3):
        s = state[i]
        newstate = np.array(s)
        newstate = np.reshape(newstate, (-1, SCREEN_WIDTH))
        newstate = np.transpose(newstate)
    
        for i in range(SCREEN_WIDTH):
            newstate[i] = newstate[i][::-1]
        
        arr.append(newstate)

    #print(newstate)
    return arr

def train(IS_IMPORT):
    if IS_IMPORT == True:
        fs = FileLoad('F:\work\cocos\dqnTest\Resources\scenario - Copy.sce')
    else:        
        server.accept()

    print('Loading ...')
    sess = tf.Session()

    # 다음에 취할 액션을 DQN 을 이용해 결정할 시기를 결정합니다.
    epsilon = 1.0
    # 프레임 횟수
    time_step = 0
    global_step = tf.Variable(0, trainable=False, name='global_step')

    brain = DQN(sess, SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ACTION, global_step)
    #brain = DQN(sess, 61, global_step)

    rewards = tf.placeholder(tf.float32, [None])
    tf.summary.scalar('avg.reward/ep.', tf.reduce_mean(rewards))
    totalScores = tf.placeholder(tf.float32, [None])
    tf.summary.scalar('avg.totalScore/ep.', tf.reduce_mean(totalScores))

    total_reward_list = []
    total_score_list = []

    saver = tf.train.Saver(tf.global_variables())

    if brain.DUELING_DQN == True:
        ckpt = tf.train.get_checkpoint_state('./model_dueling')
        writer = tf.summary.FileWriter('logs_dueling', sess.graph)
    else:
        ckpt = tf.train.get_checkpoint_state('./model')
        writer = tf.summary.FileWriter('logs', sess.graph)

    if ckpt and tf.train.checkpoint_exists(ckpt.model_checkpoint_path):
        saver.restore(sess, ckpt.model_checkpoint_path)
    else:
        sess.run(tf.global_variables_initializer())
    
    summary_merged = tf.summary.merge_all()

    brain.update_target_network()
    print('global_step:', sess.run(global_step))
    # 게임을 시작합니다.
    for episode in range(MAX_EPISODE):
        terminal = False
        total_reward = 0
        weight = 0

        # 게임을 초기화하고 현재 상태를 가져옵니다.
        # 상태는 screen_width x screen_height 크기의 화면 구성입니다.
        #state = game.reset()
        if IS_IMPORT:
            id, _, _, _, state = fs.readState()
            if id == -1:
                sys.exit(1)
        else:
            id, _, _, _, state = server.readStatus()

        if id == -1:
            continue
        
        state = reshapeFromPacket(state)
        state.append(state[2])
        state.append(state[2])
                
        
        brain.init_state(state)
        
        while not terminal:                
            actionType = "Action:"

            if IS_IMPORT:
                action = fs.readAction()
                if action == -1: sys.exit(1)
                id, reward, totalScore, terminal, state = fs.readState()
                if id == -1: sys.exit(1)
            else:                
                
                if np.random.rand() < epsilon:
                    #action = random.randrange(NUM_ACTION)
                    #print("Random action:", action)                
                    action = -1
                    #action = random.uniform(-1, 1)
                else:
                    action = brain.get_action()
                
                #action = brain.get_action()

                if episode > OBSERVE:
                    epsilon -= 1 / 1000

                # 결정한 액션을 이용해 게임을 진행하고, 보상과 게임의 종료 여부를 받아옵니다.
                #state, reward, terminal = game.step(action)
            
                server.sendX(id, action)
            
                
                if action == -1:
                    id2, action = server.readAction()
                    actionType = "Random Action:"

                    if id != id2:
                        print("Invalid Packet", id, id2)

            
                id, reward, totalScore, terminal, state = server.readStatus()

            reward = reward + (weight * 0.1)
            weight = weight + 1
            
            print(time.strftime("%H:%M:%S", time.localtime()), id, actionType,  action, "totalScore:", totalScore, "reward:", reward, "terminal", terminal)

            if id == -1:
                break
            if terminal == True:
                total_score_list.append(totalScore)

            state = reshapeFromPacket(state)
            
            total_reward += reward

            # 현재 상태를 Brain에 기억시킵니다.
            # 기억한 상태를 이용해 학습하고, 다음 상태에서 취할 행동을 결정합니다.
            brain.remember(state, action, reward, terminal)

            if time_step > OBSERVE and time_step % TRAIN_INTERVAL == 0:
                # DQN 으로 학습을 진행합니다.
                try:
                    brain.train()
                except:
                    print("Train Error!!")
                    time_step -= 1

            if time_step % TARGET_UPDATE_INTERVAL == 0:
                # 타겟 네트웍을 업데이트 해 줍니다.
                brain.update_target_network()

            time_step += 1
            

        print('\t Count of Play: %d Score: %d' % (episode + 1, total_reward))

        total_reward_list.append(total_reward)

        
        if (episode) % 10 == 0:
            summary = sess.run(summary_merged, feed_dict={rewards: total_reward_list, totalScores: total_score_list})
            writer.add_summary(summary, sess.run(global_step))
            total_reward_list = []
            total_score_list = []

        if (episode+1) % 100 == 0:
            if brain.DUELING_DQN == True:
                saver.save(sess, 'model_dueling/dqn.ckpt', global_step=global_step)
            else:
                saver.save(sess, 'model/dqn.ckpt', global_step=global_step)


def replay():
    print('Loading..')
    sess = tf.Session()
    
    global_step = tf.Variable(0, trainable=False, name='global_step')
    brain = DQN(sess, SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ACTION, global_step)
    #brain = DQN(sess, SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ACTION)

    saver = tf.train.Saver()
    if brain.DUELING_DQN == True:
        ckpt = tf.train.get_checkpoint_state('model_dueling')
    else:
        ckpt = tf.train.get_checkpoint_state('model')
    saver.restore(sess, ckpt.model_checkpoint_path)

    server.accept()

    # 게임을 시작합니다.
    for episode in range(MAX_EPISODE):
        terminal = False
        total_reward = 0

        #state = game.reset()
        id, _, _, _, state = server.readStatus()
        state = reshapeFromPacket(state)
        brain.init_state(state)

        while not terminal:
            
            action = brain.get_action()

            # 결정한 액션을 이용해 게임을 진행하고, 보상과 게임의 종료 여부를 받아옵니다.
            #state, reward, terminal = game.step(action)
            server.sendX(id, action)
            id, reward, totalScore, terminal, state = server.readStatus()
            state = reshapeFromPacket(state)
            total_reward += reward

            brain.remember(state, action, reward, terminal)

            # 게임 진행을 인간이 인지할 수 있는 속도로^^; 보여줍니다.
            #time.sleep(0.3)

        print('Count of Play: %d total reward: %d' % (episode + 1, total_reward), "Action", action)


def main(_):
    isTrain = True
    #if FLAGS.train:
    if isTrain:
        train(False)
    else:
        replay()
    #train()

if __name__ == '__main__':
    tf.app.run()