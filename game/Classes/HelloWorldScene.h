/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "library/PhysicsScene.h"
#include <mutex>
#define CAPTURE_SIZE 3
class HelloWorld : public PhysicsScene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init();
    // implement the "static create()" method manually
    CREATE_FUNC_PHYSICS(HelloWorld);
private:
	struct _capture {
		float array[5 * (12 + 1)];
		_capture() {
			memset(&array, 0, sizeof(array));
		}
		
	};

    bool onTouchBegan(Touch* touch, Event* event) ;
    bool onTouchEnded(Touch* touch, Event* event);
    void onTouchMoved(Touch *touch, Event *event);
    bool onContactBegin(PhysicsContact &contact);
    
    Node * createBall();
	Node * createBoard(int id, bool isL);
	void execute();
    Vec2 getVelocity(float movedX);
    
    void removeObstacle(int tag);
	Vec2 deleteObstacle(int tag);
    bool addObstacle();
	void addObstacleAsync(float f = 0);

    void makeBound();
    void createLayer();
    Sprite * getObstacle();
    
    void initLayer();
	void reset();
	void start(float f = 0);
	void move(int n);
	void update(float f);
    
    void run(bool isTerminal);
    void increaseScore();

	void capture(_capture * buf);	
	//int mCaptureCnt;
    
	float mMinY;
    std::mutex mLock;
    Node * mBottom;
    Node * mBall, * mCoin;
    Node * mLine;
	Node * mBoardL, *mBoardR;
    Label * mScoreLabel;
	int mTemp;	
    int mScore, mScoreTotal, mReward;
	int mRewardAccumul;
    Vec2 mStartPos;	
	Size mGridNodeSize;
    
    float mRadius;
    float mDefaultVelocity;
    LayerColor * mLayer;
    Size mGridSize;
	enum state {
		READY,		
		SENDING,		
	};
	state mState;
    
    int mSeq;
    struct _pos {
        int x;
        int y;
    };
    map<int, _pos> mMap;
    queue<Sprite*> mQueue;
    
#pragma pack(push, 1)
    struct _packet{
        int id;		
        int reward;
        int totalReward;
        int isTerminal;
		//_capture cap[CAPTURE_SIZE];
		_capture cap1;
		_capture cap2;
		_capture cap3;

    };
#pragma pack(pop)
	_packet mPacket;
	
    int createSocket(const char * host, unsigned short port);
    void sendRecv(_packet & packet);	
	int recvAI();
	int getAI(float x);	
	int getAI();

    int mSocket;
    int mSendSeq;
	int mCurrentAIVal;
    int mTestDegree;
    
    std::vector<_capture> mCaptures;

	int mDeadLineGrid;
    float mDeadLine;
	float mDecideLine;

	int getEntropy(float cnt, float total);
	float mTotal;	

	FILE *mF;
	int mCurrentType;
	void openFile() {
		mF = fopen("./scenario.sce", "wb");
	};
	void closeFile() {
		fclose(mF);
	}
	void writeFileAction(int * action) {
		if (mCurrentType == 2)
			return;
		mCurrentType = 2;
		fwrite(&mCurrentType, 1, sizeof(int), mF);
		fwrite(action, sizeof(int), 1, mF);
	}
	void writeFile() {	
	   	if (mCurrentType == 1 && mPacket.isTerminal) {
  			writeFileAction(&mCurrentAIVal);
		}
		mCurrentType = 1;
		fwrite(&mCurrentType, 1, sizeof(int), mF);
		size_t size = fwrite(&mPacket, 1, sizeof(mPacket), mF);
	}

	void readFileTest() {		
		FILE *fp = fopen("./scenario.sce", "rb");
		
		if(fp) {
			while (true) {
				size_t size;
				int type;
				size = fread(&type, 1, sizeof(int), fp);
				if (type == 1) {
					_packet p;
					size = fread(&p, 1, sizeof(_packet), fp);
					if (size < sizeof(_packet))
						break;
					CCLOG("[type: %d] id: %d, teminal: %d, reward: %d, x3: %f, y3: %f", type, p.id, p.isTerminal, p.reward, p.cap3.array[60], p.cap3.array[61]);
				}
				else {
					int n;
					size = fread(&n, 1, sizeof(int), fp);
					if (size < sizeof(int))
						break;
					CCLOG("[type: %d] action: %d", type, n);
				}

				
			}
			
			fclose(fp);
		}
	}
};

#endif // __HELLOWORLD_SCENE_H__
