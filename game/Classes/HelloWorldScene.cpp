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

#include "HelloWorldScene.h"
#include "library/util.h"
//#include "tensorflow/lite/main.h"

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#define read(a,b,c)		recv(a,b,c,0)
#define write(a,b,c)	send(a,b,c,0)
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


#include <math.h>

USING_NS_CC;

#define BOTTOM_ID 0
#define WALL_ID 1
#define BOARD_ID_L 2
#define BOARD_ID_R 3
#define SEPPED 2
#define ACTION_MAX 36
#define PHYSICSMATERIAL PhysicsMaterial(0.5, 1, 0.5)
#define START this->scheduleOnce(schedule_selector(HelloWorld::start), 0.1f)
#define ADDOBSTACLE this->scheduleOnce(schedule_selector(HelloWorld::addObstacleAsync), 0.1f)

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
#ifdef WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return false;
	}
#endif
//    readFileTest();
//    openFile();
    
    
    
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }
    //tf
    mTflitePath = FileUtils::getInstance()->fullPathForFilename("dqn.tflite");
    mTflite.init(mTflitePath.c_str());
    
    mTestDegree = 0;
    //init socket
    mSocket = createSocket("127.0.0.1", 50008);
    mGridSize = Size(5, 12);	
    
	mTotal = 0;
	for (int n = 0; n < mGridSize.height; n++) {
		mTotal += n * 2;
		if(n >= mGridSize.height - 2)
			mTotal += n * 2;
	}
    gui::inst()->init();
	//796
	int size = sizeof(mPacket);
    
    //init var
	mLayer = NULL;
    mLine = NULL;
    mScore = 0;
    mReward = 0;
    mRewardAccumul = 0;
	mBall = NULL;	

    TOUCH_INIT(HelloWorld);
    PHYSICS_CONTACT(HelloWorld);
    
    mScoreLabel = gui::inst()->addLabel(4, 0, "0", this, 0, ALIGNMENT_CENTER, Color3B::GRAY);	
    initLayer();
	/*
	mTemp = 0;
	gui::inst()->addTextButton(0, 0, "Start", this, [=](Ref* pSender) {		
		move((mTemp % ((int)mGridSize.width + 1)) + 1);
		mTemp++;
	});
	*/
	initPhysicsBody(mLayer, PHYSICSMATERIAL, false, SEPPED);
	//this->schedule(schedule_selector(HelloWorld::update), std::min(0.05f, (1.f / SEPPED)));
	this->scheduleUpdate();
    return true;
}

void HelloWorld::createLayer() {
	float width = gui::inst()->mVisibleX * 0.7;	
	mLayer = LayerColor::create(Color4B::BLACK, width, width * 1.5);
	mDefaultVelocity = mLayer->getContentSize().height * 1.2;
	Vec2 pos = gui::inst()->getCenter();
	pos.x -= mLayer->getContentSize().width / 2;
	pos.y -= mLayer->getContentSize().height / 2;
	mLayer->setPosition(pos);
	CCLOG("mLayer size = (%f, %f)", mLayer->getContentSize().width, mLayer->getContentSize().height);	
	
	Vec2 v1 = gui::inst()->getPointVec2(0, 0, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
	Vec2 v2 = gui::inst()->getPointVec2(0, 1, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
	Vec2 v3 = gui::inst()->getPointVec2(1, 0, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
	mRadius = (v1.y - v2.y) * 0.5;
	
	mGridNodeSize = Size((v3.x - v1.x), (v1.y - v2.y));	
	this->addChild(mLayer);
}

void HelloWorld::initLayer() {
	if (mLayer) {
		this->removeChild(mLayer);
	}

	createLayer();
    makeBound();
    gui::inst()->drawGrid(mLayer, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
    //ball
	mBoardL = createBoard(BOARD_ID_L, true);
	mBoardR = createBoard(BOARD_ID_R, false);
    //deadline
	mDeadLineGrid = mGridSize.height / 2 - 1;
	Vec2 p = gui::inst()->getPointVec2(0, mDeadLineGrid, ALIGNMENT_NONE, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
	gui::inst()->addLabelAutoDimension(2, mDeadLineGrid, "X", mLayer, 0, ALIGNMENT_CENTER, Color3B::RED, mGridSize, Size::ZERO, Size::ZERO);
	mDeadLine = p.y;
	mDecideLine = gui::inst()->getPointVec2(0, mGridSize.height - 2, ALIGNMENT_NONE, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO).y;
	reset();
	START;
}

void HelloWorld::reset() {
	for (std::map<int, _pos>::iterator it = mMap.begin(); it != mMap.end(); ++it) {
		mLayer->removeChildByTag(it->first);
	}
	mMap.clear(); 
	mScoreLabel->setString(to_string(0));
	mScoreTotal = 0;
	mScore = 0;
	mSeq = 10;		
	mState = READY;
	mMinY = mLayer->getContentSize().height;
	mCaptures.clear();
	::memset(&mPacket, 0, sizeof(mPacket));
	if (mBall) {
		mLayer->removeChild(mBall);
		mBall = NULL;
	}
}

void HelloWorld::start(float f) {
	for (int n = 0; n < 3; n++)	
		addObstacle();

	createBall();
}

void HelloWorld::move(int n) {
	//CCLOG("Move to %d", n);
	float x = gui::inst()->getPointVec2(n+1, 0, ALIGNMENT_NONE, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO).x;
	mBoardL->setPosition(Vec2(x, mBoardL->getPosition().y));
	mBoardR->setPosition(Vec2(x, mBoardR->getPosition().y));
}

void HelloWorld::update(float f) {
	if (mBall == NULL)
		return;

	if (mSocket == -1) {		
//        move(getAI(mBall->getPosition().x));
	}
	
	switch (mState) {
	case SENDING:
		return;
	default:
		break;
	}

	Vec2 vel = mBall->getPhysicsBody()->getVelocity();
	if (vel.y < 0 && mDeadLine >= mBall->getPosition().y) {
		if (mBall->getPosition().y <= mDecideLine)
		{	
			if (mCaptures.size() < CAPTURE_SIZE) {
				int remain = CAPTURE_SIZE - mCaptures.size();
				for (int n = 0; n < remain; n++) {
					_capture p;
					capture(&p);
					mCaptures.push_back(p);
				}
			}
			/*
			memcpy(&mPacket.cap1, &mCaptures[0], sizeof(_capture));
			memcpy(&mPacket.cap2, &mCaptures[mCaptures.size() / 2], sizeof(_capture));
			memcpy(&mPacket.cap3, &mCaptures[mCaptures.size()-1], sizeof(_capture));
			*/
			makeReshape(mCaptures[0], mCaptures[mCaptures.size() / 2], mCaptures[mCaptures.size() - 1]);

			return run(false);
		}
		else {
			_capture p;
			capture(&p);
			mCaptures.push_back(p);
		}
	}		
}

void HelloWorld::makeReshape(_capture &cap1, _capture &cap2, _capture &cap3) {
	float c1[WIDTH][HEIGHT];
	float c2[WIDTH][HEIGHT];
	float c3[WIDTH][HEIGHT];

	memcpy(c1, &cap1.array, sizeof(c1));
	memcpy(c2, &cap2.array, sizeof(c2));
	memcpy(c3, &cap3.array, sizeof(c3));

	float temp[WIDTH][HEIGHT][CAPTURE_SIZE];
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			temp[x][y][0] = c1[x][y];
			temp[x][y][1] = c2[x][y];
			temp[x][y][2] = c3[x][y];
		}
	}

	memcpy(mPacket.cap.array, temp, sizeof(mPacket.cap));
}

Sprite * HelloWorld::getObstacle() {
    auto p = gui::inst()->addSpriteAutoDimension(0, 0, "rect.png", mLayer, ALIGNMENT_NONE, mGridSize, Size::ZERO, Size::ZERO);	
    gui::inst()->setScale(p, mRadius);
    setPhysicsBodyRect(p, PHYSICSMATERIAL, false, mSeq, 0x1 << 1, 0x1 << 0, 0x1 << 0);
    return p;
}
void HelloWorld::addObstacleAsync(float f) {
	if (!addObstacle()) {
		run(true);		
	}
}
bool HelloWorld::addObstacle() {
	vector<int> ridObstacles;
    for (std::map<int,_pos>::iterator it=mMap.begin(); it!=mMap.end(); ++it) {
        auto p = mLayer->getChildByTag(it->first);
        if(p) {
            _pos newPos;
            newPos.x = it->second.x;
            newPos.y = it->second.y + 1;
			if (newPos.y >= mDeadLineGrid)
				ridObstacles.push_back(it->first);
            //if(newPos.y >= mGridSize.height -1) return false;
			Vec2 pos = gui::inst()->getPointVec2(newPos.x, newPos.y, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
            p->setPosition(pos);
            mMap[it->first] = newPos;
			if (pos.y < mMinY)
				mMinY = pos.y;
        }
    }

	for (int n = 0; n < ridObstacles.size(); n++) {
		deleteObstacle(ridObstacles[n]);
	}

    int x = -1;
    for(int n=0; n<2; n++){
        _pos pos;
        if(x == -1){
            x = getRandValue(mGridSize.width);
        } else {
            while(true) {
                int temp = getRandValue(mGridSize.width);
                if(x != temp){
                    x = temp;
                    break;
                }
            }
        }
        pos.x = x;
        pos.y = 0;

        auto p = getObstacle();
        Vec2 posPx = gui::inst()->getPointVec2(pos.x, pos.y, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
        p->setPosition(posPx);
        p->setTag(mSeq);
        p->setColor(Color3B::GRAY);    
        p->runAction(Blink::create(0.5, 2));		
        mMap[mSeq] = pos;
        mSeq++;
    }
    return true;
}

Vec2 HelloWorld::deleteObstacle(int tag) {
	Node * node = mLayer->getChildByTag(tag);
	if (node) {
		Vec2 pos = node->getPosition();
		mMap.erase(tag);
		mLayer->removeChildByTag(tag);
		return pos;
	}
	else {
		return Vec2::ZERO;
	}
}

void HelloWorld::removeObstacle(int tag) {    
	Vec2 pos = deleteObstacle(tag);
    if(pos != Vec2::ZERO) {
        string sz = to_string(mScore + 1) + " COMBO";
        auto label = gui::inst()->addLabelAutoDimension(0, 0, sz, mLayer, 10, ALIGNMENT_CENTER, Color3B::ORANGE);
        label->setPosition(pos);
        label->runAction( Sequence::create(ScaleBy::create(0.5, 1.5), RemoveSelf::create(), NULL) );        
		//score		
		increaseScore();
		mScoreLabel->setString(to_string(mScoreTotal));
    }
	float min = mLayer->getContentSize().height;
	for (std::map<int, _pos>::iterator it = mMap.begin(); it != mMap.end(); ++it) {
		Vec2 pos = gui::inst()->getPointVec2(it->second.x, it->second.y, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
		if (pos.y < min)
			min = pos.y;
	}
	mMinY = min;
}

void HelloWorld::makeBound() {
    float thick = 1;    
    //bottom
    mBottom = gui::inst()->addSprite(0, 5, "rect.png", mLayer, ALIGNMENT_NONE, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
    mBottom->setAnchorPoint(Vec2(0, 0));
    mBottom->setPosition(Vec2::ZERO);
    mBottom->setContentSize(Size(mLayer->getContentSize().width, thick));
    setPhysicsBodyRect(mBottom, PhysicsMaterial(1,0,1), false, BOTTOM_ID, 0x1 << 2, 0x1 << 0, 0x1 << 0);    
}

Node * HelloWorld::createBall() {
	this->getPhysicsWorld()->setAutoStep(false);
	this->getPhysicsWorld()->step(0.0f);
	if (mBall) {
		mLayer->removeChild(mBall);
	}
    auto ball = gui::inst()->addSpriteAutoDimension(2, 5, "circle.png", mLayer, ALIGNMENT_CENTER, mGridSize, Size::ZERO, Size::ZERO);
	Vec2 pos = gui::inst()->getPointVec2(getRandValue(mGridSize.width), 5, ALIGNMENT_CENTER, mLayer->getContentSize(), mGridSize, Size::ZERO, Size::ZERO);
	ball->setPosition(pos);
    gui::inst()->setScale(ball, mRadius);    
	this->setPhysicsBodyCircle(ball, PHYSICSMATERIAL, true, 1, 0x1 << 0, 0x1 << 1 | 0x1 << 2, 0x1 << 1 | 0x1 << 2);
    ball->getPhysicsBody()->setVelocityLimit(mDefaultVelocity * 1.f);
	//ball->getPhysicsBody()->setVelocity(Vec2(0, mDefaultVelocity));
	mBall = ball;
	this->getPhysicsWorld()->setAutoStep(true);
	//CCLOG("mBall %f, %f", mBall->getPosition().x, mBall->getPosition().y);
    return ball;
}

Node * HelloWorld::createBoard(int id, bool isL) {
	float thick = 5;
	//bottom
	Node * board = gui::inst()->addSpriteAutoDimension(2, mGridSize.height - 1, "rect.png", mLayer, ALIGNMENT_CENTER, mGridSize, Size::ZERO, Size::ZERO);
	if(isL)
		board->setAnchorPoint(Vec2(2, 0.5));	
	else
		board->setAnchorPoint(Vec2(1, 0.5));

	board->setContentSize(Size(mLayer->getContentSize().width / mGridSize.width, thick));
	setPhysicsBodyRect(board, PHYSICSMATERIAL, false, id, 0x1 << 2, 0x1 << 0, 0x1 << 0);

	return board;
}

bool HelloWorld::onTouchBegan(Touch* touch, Event* event)  {
    mStartPos = touch->getLocation();
    return true;
}
bool HelloWorld::onTouchEnded(Touch* touch, Event* event) {
	/*
    if(!mIsReady)
        return false;
    
    if(mLine)
        mLayer->removeChild(mLine);
    
    Vec2 moved = getVelocity(touch->getLocation().x);
    mScore = 0;
    mBall->getPhysicsBody()->setVelocity(moved);
    mIsReady = false;
	*/
    return true;
}
void HelloWorld::onTouchMoved(Touch *touch, Event *event) {    
    /*
    if(mLine)
        mLayer->removeChild(mLine);
    
    Vec2 moved = getVelocity(touch->getLocation().x);
    
    Vec2 start = mBall->getPosition();
    Vec2 end = Vec2(start.x + moved.x, mLayer->getContentSize().height);
    
    mLine = gui::inst()->drawLine(mLayer, start, end, Color4F::BLUE, 5);
	*/
    float x = mBoardL->getContentSize().width * 6;
    mBoardL->setPosition(Vec2(touch->getLocation().x - x, mBoardL->getPosition().y));
    mBoardR->setPosition(Vec2(touch->getLocation().x - x, mBoardR->getPosition().y));
    
}

Vec2 HelloWorld::getVelocity(float endX){
    float moved = endX - mStartPos.x;
    moved *= 10;
    float ratio =  abs(moved) / mLayer->getContentSize().width;    
    float y = mDefaultVelocity * 1.f;    
    return Vec2(moved, y);
}

bool HelloWorld::onContactBegin(PhysicsContact &contact) {
	if (mBall == NULL)
		return false;

    int other;
//    if(isCollosion(contact, 1, other)) {
//        CCLOG("Collision 1 with %d", other);
//        //        bitmask st = getBitmask(contact);
//        //        CCLOG("Category %d, %d", st.categoryA, st.categoryB);
//        //        CCLOG("Collision %d, %d", st.collisionA, st.collisionB);
//        //        CCLOG("Contact %d, %d", st.contactA, st.contactB);
//    }
    
    if(isContact(contact, 1, other)) {
//        CCLOG("Contact 1 with %d", other);
        //        bitmask st = getBitmask(contact);
        //        CCLOG("Category %d, %d", st.categoryA, st.categoryB);
        //        CCLOG("Collision %d, %d", st.collisionA, st.collisionB);
        //        CCLOG("Contact %d, %d", st.contactA, st.contactB);
		Vec2 vel = mBall->getPhysicsBody()->getVelocity();
        switch(other) {
			case BOARD_ID_L:
				mBall->getPhysicsBody()->setVelocity(Vec2((-1 * mLayer->getContentSize().width / 2), mDefaultVelocity));
				execute();
				break;
			case BOARD_ID_R:
				mBall->getPhysicsBody()->setVelocity(Vec2((1 * mLayer->getContentSize().width / 2), mDefaultVelocity));
				execute();
				break;
            case WALL_ID:
                break;
            case BOTTOM_ID:		
				run(true);							
                break;
            default:                
                removeObstacle(other);
                break;
        }
    }
    return true;
}
void HelloWorld::execute() {
	ADDOBSTACLE;
//    writeFileAction(&mCurrentAIVal);
//    CCLOG("Save action");
	mScore = 0;
	mState = READY;
}
void HelloWorld::increaseScore() {
    mLock.lock();
    mScoreTotal++;
	mScore++;
    mLock.unlock();
}
/*
	Packet
*/
void HelloWorld::capture(_capture * buf)
{
	float array[5 * (12 + 1)];
	memset(array, 0, sizeof(array));

	for (std::map<int, _pos>::iterator it = mMap.begin(); it != mMap.end(); ++it) {
		int idx = it->second.x + (it->second.y * mGridSize.width);
		array[idx] = 1;
	}
	float x, y;
	x = mBall->getPosition().x / mLayer->getContentSize().width;	
	y = mBall->getPosition().y / mLayer->getContentSize().height;

	Vec2 pos;
	pos.x = (int)(x / (1 / mGridSize.width));
	pos.y = mGridSize.height - (int)(y / (1 / mGridSize.height));

	//gui::inst()->addLabelAutoDimension(pos.x, pos.y, "O", mLayer, 0, ALIGNMENT_CENTER, Color3B::YELLOW, mGridSize, Size::ZERO, Size::ZERO)->runAction(Sequence::create(Blink::create(1, 1), RemoveSelf::create(), NULL));
	int idx = (mGridSize.width * pos.y) + pos.x;
	array[idx] = 2;
	//mRect->setPosition(posRect);
	idx = mGridSize.width * mGridSize.height;
	Vec2 velocity = mBall->getPhysicsBody()->getVelocity();
	array[idx] = velocity.x / mDefaultVelocity;
	array[idx + 1] = velocity.y / mDefaultVelocity;

	bool isReshape = true;
	//reshape
	if (isReshape) {
		float tempArr[HEIGHT][WIDTH];
		idx = 0;
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				tempArr[y][x] = array[idx];
				idx++;
			}
		}
		idx = 0;
		for (int x = 0; x < WIDTH; x++) {
			for (int y = HEIGHT - 1; y >= 0; y--) {
				buf->array[idx] = tempArr[y][x];
				idx++;
			}
		}

		//검증
		/*
		float temp[WIDTH][HEIGHT];
		idx = 0;
		for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
		temp[x][y] = buf->array[idx];
		idx++;
		}
		}
		idx = 0;
		*/
	}
	else {
		memcpy(buf->array, array, sizeof(buf->array));
	}
	
}

void HelloWorld::run(bool isTerminal) {	
	
	mState = SENDING;

    mPacket.isTerminal = isTerminal;
	if (isTerminal) {
		mPacket.reward = (mScore == 0) ? -10 : mScore;
	}
	else {
		mPacket.reward = mScore;
	}
	
	mPacket.totalReward = mScoreTotal;
	mPacket.id = mSendSeq++;    
    sendRecv(mPacket);

    if(isTerminal) {		
		//CCLOG("Run terminal id: %d, totalScore: %d", mPacket.id, mPacket.totalReward);
		reset();
		START;
		CCLOG("Init packet");
		//temp	
		//if(mSocket == -1) move(recvAI());
	}
	//else if(mSocket != -1)
    move(recvAI());
	
	
	mCaptures.clear();
	::memset(&mPacket, 0, sizeof(mPacket));
}
int HelloWorld::getAI(float x) {
    if(mBall == NULL)
        return 0;
    
	float ratio = x / mLayer->getContentSize().width;
	int n = (ratio / 0.2) + 1;	
	if (mBall->getPhysicsBody()->getVelocity().x < 0)
		n--;
	mCurrentAIVal = n;
	return n;
}


int HelloWorld::recvAI() {
	if (mSocket == -1) {
        float output[6] = {0,};
        int n = mTflite.run(mPacket.cap.array
                             , sizeof(mPacket.cap.array) / sizeof(mPacket.cap.array[0])
                             , output
                             , 6
                             , false);
        float maxVal = -1000;
        int maxIdx;
        for(int n=0; n <6; n++){
            CCLOG("%d - %f", n, output[n]);
            if(maxVal < output[n]) {
                maxVal = output[n];
                maxIdx = n;
            }
        }
        
		return maxIdx;
	}

	unsigned char buf[8] = { 0, };
	long recvSize = 0;
	while (true) {
		long size = read(mSocket, (char*)buf, sizeof(buf));
		if (size == -1) {
			CCLOG("RECV Error");
			return -1;
		}
		recvSize += size;
		if (recvSize >= 8) {

			break;
		}
		else {
			CCLOG("Recv more %d", (int)recvSize);
		}
	}
	struct _packetRecv {
		int id;
		int x;
	} packetRecv;

	memcpy(&packetRecv, buf, sizeof(packetRecv));
	if(packetRecv.x != -1)
		return packetRecv.x;
	
	// 클라에서 판단
	int x = getRandValue(6);
	_packetRecv p;
	p.id = packetRecv.id;
	p.x = x;
	long sizeSend = write(mSocket, (const char *)&p, sizeof(p));
	return x;
}

void HelloWorld::sendRecv(_packet & packet) {
	if (mSocket == -1) {
//        writeFile();
		return;
	}

    while(true) {
        long sizeSend = write(mSocket, (const char *)&packet , sizeof(packet));
//        CCLOG("Send (%d) id: %d", (int)sizeSend, packet.id);
        
        unsigned char buf[12] = {0,};
        long retSize = 0;
        while(true){
            long size = read(mSocket, (char*)buf, sizeof(buf));
            if(size > 0) {
                retSize += size;
                if(retSize == 12) {
                    int id1, id2, id3;
                    memcpy(&id1, buf, 4);
                    memcpy(&id2, buf + 4, 4);
                    memcpy(&id3, buf + 8, 4);
                    if(packet.id == id1 && id1 == id2 && id2 == id3) {
//                        CCLOG("Recv ack %d, %d, %d, %d", packet.id, id1, id2, id3);						
                        return;
                    }
                    else {
                        CCLOG("Recv Invalid id %d, %d, %d, %d", packet.id, id1, id2, id3);
                        break;
                    }
                }
            } else {
                CCLOG("Recv Timeout %d", (int)retSize);
                break;
            }
        }
    }
}

int HelloWorld::createSocket(const char * host, unsigned short port) {
    mSendSeq = 0;
    struct sockaddr_in clientaddr;
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_addr.s_addr = inet_addr(host);
    clientaddr.sin_port = htons(port);
    
    int client_len = sizeof(clientaddr);
    
    if (connect(client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0)
    {
        CCLOG("Connect error: ");
        return -1;
    }
    /*
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    */
    return client_sockfd;
}

int HelloWorld::getEntropy(float cnt, float total) {
	if (cnt == 0)
		return 10000;
	float el = (total - cnt);
	float v1 = cnt / total;
	float log_1 = log2(v1);
	float v2 = el / total;
	float log_2 = log2(v2);
	float entropy = (-1 * (v1*log_1)) + (-1 * (v2*log_2));
	CCLOG("getEntropy cnt = %f", cnt);
	return floorf((1 + (1 - entropy)) * 1000) ;
}
