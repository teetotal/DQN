# -*- coding: utf-8 -*-

import tensorflow as tf
import numpy as np
import random
from collections import deque
import tensorflow.contrib.slim as slim

class DQN:
    # 학습에 사용할 플레이결과를 얼마나 많이 저장해서 사용할지를 정합니다.
    # (플레이결과 = 게임판의 상태 + 취한 액션 + 리워드 + 종료여부)
    REPLAY_MEMORY = 10000
    # 학습시 사용/계산할 상태값(정확히는 replay memory)의 갯수를 정합니다.
    BATCH_SIZE = 4 #32
    TRAIN_LENGTH = 8
    # 과거의 상태에 대한 가중치를 줄이는 역할을 합니다.
    GAMMA = 0.99
    #GAMMA = 0.8
    # 한 번에 볼 총 프레임 수 입니다.
    # 앞의 상태까지 고려하기 위함입니다.
    STATE_LEN = 3
    #STATE_LEN = 1

    DOUBLE_DQN = True
    DUELING_DQN = True
    DRQN = True

    def __init__(self, session, width, height, n_action, global_step):
        self.session = session
        self.n_action = n_action
        self.width = width
        self.height = height
        # 게임 플레이결과를 저장할 메모리
        #self.memory = deque()
        self.memory = []
        # 현재 게임판의 상태
        self.state = None

        # 게임의 상태를 입력받을 변수
        # [게임판의 가로 크기, 게임판의 세로 크기, 게임 상태의 갯수(현재+과거+과거..)]
        #self.X =  tf.placeholder(shape=[None, width * height * self.STATE_LEN],dtype=tf.float32)
        #self.input_X = tf.reshape(self.X, shape=[-1, width, height, self.STATE_LEN])
        self.input_X = tf.placeholder(tf.float32, [None, width, height, self.STATE_LEN])
        # 각각의 상태를 만들어낸 액션의 값들입니다. 0, 1, 2 ..        
        self.input_A = tf.placeholder(tf.int32, [None])
        # 손실값을 계산하는데 사용할 입력값입니다. train 함수를 참고하세요.
        self.input_Y = tf.placeholder(tf.float32, [None])

        #self.input_Batch = tf.placeholder(tf.int32, [None])
        self.trainLength = tf.placeholder(dtype=tf.int32)
        self.input_Batch = tf.placeholder(dtype=tf.int32,shape=[])

        self.Q = self._build_network('main')
        self.cost, self.train_op = self._build_op(global_step)

        # 학습을 더 잘 되게 하기 위해,
        # 손실값 계산을 위해 사용하는 타겟(실측값)의 Q value를 계산하는 네트웍을 따로 만들어서 사용합니다
        self.target_Q = self._build_network('target')

    def _build_network(self, name):
        with tf.variable_scope(name):
            model = tf.layers.conv2d(self.input_X, 32, [1, 1], strides=(1, 1), padding='same', activation=tf.nn.relu)            
            model = tf.layers.conv2d(model, 64, [2, 2], strides=(1, 1), padding='same', activation=tf.nn.relu)
            model = tf.layers.conv2d(model, 64, [5, 1], strides=(1, 1), padding='same', activation=tf.nn.relu)
            ''' 
            model = slim.convolution2d( inputs=model
                        ,num_outputs=512
                        , kernel_size=[1,1]
                        ,stride=[1,1]
                        , padding='VALID'
                        , biases_initializer=None
                        , scope=name+'_last_conv')
            '''
            if self.DRQN == True:
                hiddens = 512 # 5*832 = 4016 = 5*13*64
                model = tf.layers.conv2d(model, hiddens, [self.width, self.height], strides=(1, 1), padding='VALID', activation=tf.nn.relu)
                #rnn 
                cell = tf.contrib.rnn.BasicLSTMCell(num_units=hiddens, state_is_tuple=True)
                model = tf.reshape(slim.flatten(model), [self.input_Batch, self.trainLength, hiddens])
                rnn, rnn_state = tf.nn.dynamic_rnn(inputs=model
                    , cell=cell
                    , dtype=tf.float32
                    , initial_state=cell.zero_state(self.input_Batch, tf.float32))
                model = tf.reshape(rnn,shape=[-1, hiddens])                
            else:
                model = tf.contrib.layers.flatten(model)                

            if self.DUELING_DQN == False:
                model = tf.layers.dense(model, 512, activation=tf.nn.relu)
                Q = tf.layers.dense(model, self.n_action, activation=None)
            else:
                val = tf.layers.dense(model, 512, activation=tf.nn.relu)            
                adv = tf.layers.dense(model, 512, activation=tf.nn.relu)

                val = tf.layers.dense(val, 1, activation=None)
                adv = tf.layers.dense(adv, self.n_action, activation=None)

                Q = val + tf.subtract(adv, tf.reduce_mean(adv, axis=1, keepdims=True))

        return Q

    def _build_op(self, global_step):
        # DQN 의 손실 함수를 구성하는 부분입니다. 다음 수식을 참고하세요.
        # Perform a gradient descent step on (y_j-Q(ð_j,a_j;θ))^2
        one_hot = tf.one_hot(self.input_A, self.n_action, 1.0, 0.0)
        Q_value = tf.reduce_sum(tf.multiply(self.Q, one_hot), axis=1)
        cost = tf.reduce_mean(tf.square(self.input_Y - Q_value))
        train_op = tf.train.AdamOptimizer(1e-6).minimize(cost, global_step=global_step)

        return cost, train_op
            

    # refer: https://github.com/hunkim/ReinforcementZeroToAll/
    def update_target_network(self):
        copy_op = []

        main_vars = tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES, scope='main')
        target_vars = tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES, scope='target')

        # 학습 네트웍의 변수의 값들을 타겟 네트웍으로 복사해서 타겟 네트웍의 값들을 최신으로 업데이트합니다.
        for main_var, target_var in zip(main_vars, target_vars):
            copy_op.append(target_var.assign(main_var.value()))

        self.session.run(copy_op)

    def get_action(self):
        Q_value = self.session.run(self.Q,
                                   feed_dict={
                                       self.input_X: [self.state]
                                       , self.input_Batch: 1
                                       , self.trainLength: 1
                                   })

        action = np.argmax(Q_value[0])

        return action
    '''
    def init_state(self, state):
        # 현재 게임판의 상태를 초기화합니다. 앞의 상태까지 고려한 스택으로 되어 있습니다.
        state = [state for _ in range(self.STATE_LEN)]
        # axis=2 는 input_X 의 값이 다음처럼 마지막 차원으로 쌓아올린 형태로 만들었기 때문입니다.
        # 이렇게 해야 컨볼루션 레이어를 손쉽게 이용할 수 있습니다.
        # self.input_X = tf.placeholder(tf.float32, [None, width, height, self.STATE_LEN])
        self.state = np.stack(state, axis=2)
    '''
    def init_state(self, state):        
        # 현재 게임판의 상태를 초기화합니다. 앞의 상태까지 고려한 스택으로 되어 있습니다.
        #state = [state for _ in range(self.STATE_LEN)]
        # axis=2 는 input_X 의 값이 다음처럼 마지막 차원으로 쌓아올린 형태로 만들었기 때문입니다.
        # 이렇게 해야 컨볼루션 레이어를 손쉽게 이용할 수 있습니다.
        # self.input_X = tf.placeholder(tf.float32, [None, width, height, self.STATE_LEN])
        self.state = np.stack(state, axis=2)

    def remember(self, state, action, reward, terminal):                
        for i in range(3):
            next_state = np.reshape(state[i], (self.width, self.height, 1))
            next_state = np.append(self.state[:, :, 1:], next_state, axis=2)
        # 플레이결과, 즉, 액션으로 얻어진 상태와 보상등을 메모리에 저장        
        self.memory.append((self.state, next_state, action, reward, terminal))

        # 저장할 플레이결과의 갯수를 제한합니다.
        if len(self.memory) > self.REPLAY_MEMORY:
            self.memory = self.memory[1:]
            #self.memory.popleft()

        self.state = next_state
    '''
    def remember(self, state, action, reward, terminal):
        # 학습데이터로 현재의 상태만이 아닌, 과거의 상태까지 고려하여 계산하도록 하였고,
        # 이 모델에서는 과거 3번 + 현재 = 총 4번의 상태를 계산하도록 하였으며,
        # 새로운 상태가 들어왔을 때, 가장 오래된 상태를 제거하고 새로운 상태를 넣습니다.
        next_state = np.reshape(state, (self.width, self.height, 1))
        next_state = np.append(self.state[:, :, 1:], next_state, axis=2)

        # 플레이결과, 즉, 액션으로 얻어진 상태와 보상등을 메모리에 저장합니다.
        self.memory.append((self.state, next_state, action, reward, terminal))

        # 저장할 플레이결과의 갯수를 제한합니다.
        if len(self.memory) > self.REPLAY_MEMORY:
            self.memory.popleft()

        self.state = next_state
    '''
    def _sample_memory(self):
        sample_memory = random.sample(self.memory, self.BATCH_SIZE)

        state = [memory[0] for memory in sample_memory]
        next_state = [memory[1] for memory in sample_memory]
        action = [memory[2] for memory in sample_memory]
        reward = [memory[3] for memory in sample_memory]
        terminal = [memory[4] for memory in sample_memory]

        return state, next_state, action, reward, terminal

    def getSampleForDRQN(self):
        #BATCH_SIZE만큼 가져 오는데
        #TRAIN_LENGTH 만큼씩 묶어서 
        temp = []

        state = []
        next_state = []
        action = []
        reward = []
        terminal = []

        for _ in range(self.BATCH_SIZE):            
            idx = np.random.randint(0,len(self.memory)+1 - self.TRAIN_LENGTH)
            memory = self.memory[idx: idx + self.TRAIN_LENGTH]
            temp.append(memory)
            
        sample_memory = np.reshape(temp,[self.BATCH_SIZE * self.TRAIN_LENGTH,5])

        state.append([memory[0] for memory in sample_memory])
        next_state.append([memory[1] for memory in sample_memory])  
        action.append([memory[2] for memory in sample_memory])
        reward.append([memory[3] for memory in sample_memory])
        terminal.append([memory[4] for memory in sample_memory])
        '''
        state = np.reshape(state, [self.BATCH_SIZE * self.TRAIN_LENGTH])
        next_state = np.reshape(next_state, [self.BATCH_SIZE * self.TRAIN_LENGTH])
        action = np.reshape(action, [self.BATCH_SIZE * self.TRAIN_LENGTH])
        reward = np.reshape(reward, [self.BATCH_SIZE * self.TRAIN_LENGTH])
        terminal = np.reshape(terminal, [self.BATCH_SIZE * self.TRAIN_LENGTH])
        '''
        return state[0], next_state[0], action[0], reward[0], terminal[0]



    def train(self):
        # 게임 플레이를 저장한 메모리에서 배치 사이즈만큼을 샘플링하여 가져옵니다.
        #state, next_state, action, reward, terminal = self._sample_memory()
        state, next_state, action, reward, terminal = self.getSampleForDRQN()

        # 학습시 다음 상태를 타겟 네트웍에 넣어 target Q value를 구합니다        
        if self.DOUBLE_DQN == False:
            target_Q_value = self.session.run(self.target_Q
            , feed_dict={
                self.input_X: next_state
                , self.input_Batch: self.BATCH_SIZE
                , self.trainLength: self.TRAIN_LENGTH
                })
        else:
            #DoubleDQN
            Q_value = self.session.run(self.Q
            , feed_dict={
                self.input_X: next_state
                , self.input_Batch: self.BATCH_SIZE
                , self.trainLength: self.TRAIN_LENGTH
                })

            target_Q_value = self.session.run(self.target_Q
            , feed_dict={
                self.input_X: next_state
                , self.input_Batch: self.BATCH_SIZE
                , self.trainLength: self.TRAIN_LENGTH
                })

        # DQN 의 손실 함수에 사용할 핵심적인 값을 계산하는 부분입니다. 다음 수식을 참고하세요.
        # if episode is terminates at step j+1 then r_j
        # otherwise r_j + γ*max_a'Q(ð_(j+1),a';θ')
        # input_Y 에 들어갈 값들을 계산해서 넣습니다.
        Y = []
        for i in range(self.BATCH_SIZE * self.TRAIN_LENGTH):
            if terminal[i]:
                Y.append(reward[i])
            else:
                if self.DOUBLE_DQN == False:
                    Y.append(reward[i] + self.GAMMA * np.max(target_Q_value[i]))
                else:
                    #DoubleDQN
                    a = np.argmax(Q_value[i])
                    Y.append(reward[i] + self.GAMMA * target_Q_value[i][a])

        self.session.run(self.train_op,
                         feed_dict={
                             self.input_X: state
                             , self.input_A: action
                             , self.input_Y: Y
                             , self.input_Batch: self.BATCH_SIZE
                             , self.trainLength: self.TRAIN_LENGTH
                         })