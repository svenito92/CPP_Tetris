/*
 * Looper.h
 *
 *  Created on: May 29, 2022
 *      Author: diktux
 */

#include <stdint.h>
#include "Playground.h"
#include "Block.h"
#include "Calculations.h"
#include "TestFile.h"
#include "st7735.h"
#include "main.h"
#include <Adafruit_TFTShield18.h>
#include "TFT_Functions.h"
#include <iostream>
#include "usart.h"
#include "mqtt_intercom.h"

#ifndef SRCCPP_LOOPER_H_
#define SRCCPP_LOOPER_H_

class Looper
{
public:
  Looper();
  virtual ~Looper();
  void run();
  bool gameStartFlag = false;
  bool interCoreComReady = false;
  uint8_t playerIds[20] = {0};
  uint8_t gameOverPlayerIds[20] = {0};
  uint8_t gameWonPlayerId = 0;
  bool gameWonFlag=false;
  uint32_t toInsertLines=0;
  uint8_t playerNr = 2; // Player 1 = Master

  // Enums
  enum ProcessState
  {
    init = 0,
    selectGameModeSetScreen = 10,
	selectGameModeBtnIn = 15,
    gameSettingsSpSetScreen = 20,
	gameSettingsSPSetLevel = 25,
    gameSettingsMp = 30,
	gameSettingsMpDrawScreen = 31,
	waitOnStart =32,
	initializeComAndScreen = 33,
    singlePlayer = 40,
    multiPlayer = 50,
    gameOver = 60,
    gameWon = 70,
    ranking = 80,
    testMode = 255
  };

  enum GameState
  {
    startGame = 0,
    generateNewBlock = 10,
    blockDown = 20,
    moveBlock = 40,
    rotateBlock = 50,
    idle = 60,
    fixBlock = 100,
    killLine = 110,
    insertLine = 120,
	spawnblock = 130,
	updateScreen =140
  };

  enum BlockType
  {
    empty = 0,
	spaghetti = 1,
	square = 2,
	invZ = 3,
	correctZ = 4,
	invL = 5,
	correctL = 6,
	pyramid = 7
  };

  enum ButtonType{
	  x = 0,
	  right = 1,
	  left = 2,
	  down = 3,
	  y = 4
  };

private:
  // Variables
  // Testmode
  TestFile test = TestFile();

  // general
  Calculations calculations = Calculations();
  uint32_t buttons;
  uint8_t buttonUpHold;
  uint8_t buttonDownHold;
  uint8_t buttonRightHold;
  uint8_t buttonAHold;
  uint8_t buttonCHold;

  Adafruit_TFTShield18 ss;

  // FSM
  ProcessState processState = init;
  GameState gameState = startGame;

  // timer and timer references
  uint32_t updateScreenTime = 51; // in ms
  uint32_t moveBlockTimer = 80;	// in ms
  const uint32_t INIT_BLOCK_DOWN_CNT = 1000; // 1000ms
  uint32_t blockDownCnt = INIT_BLOCK_DOWN_CNT; // numbers of moves before move block one field down
  uint32_t timer=0;		// when timer "overflows" move block
  uint32_t counter=0;		// counts how often the move timer overflowed
  uint32_t updateScreenCounter=0;
  uint32_t gameWonCheckCounter=0;
  uint32_t playerIdUpdate=0;
  uint32_t gameOverUpdate=0;
  uint16_t scoreRanking[10];

  // game control
  uint16_t score = 0;
  //uint8_t scoreMultiplier = 1;	//aka. Level use blockLevel cnt
  uint16_t killedLines;
  uint16_t openKilledLines;
  uint16_t blocksInGame;
  uint16_t blocksPerTypeInGame[7];
  bool gameRunning;
  uint8_t role; // 1 = Master, 2 = Player
  bool roleMenu;
  uint32_t insertedLines=0;

  uint8_t buttonPressed;
  uint8_t activePlayers=0;
  uint8_t gameOverPlayers=0;

  uint8_t gameMode=0; // 1 = Singelplayer, 2 = Multiplayer
  uint8_t holdMPGameOver=0;
  uint8_t updateSpecView=0;
  // bool moveBlockOnBottom = true;

  // blocks & playgrounds
  Playground playground = Playground();
  Block playBlocks[5];
  uint8_t currentBlockNo = 0;
  uint8_t nextBlockNo = 1;


  // Methods
  void initScreen();
  void runGame();
  void generateBlocks();
  void btnReleased(uint32_t pressedButton);
  void accelerateGame();
  void setNewScoreInRanking(uint16_t score);

  // action in states
  void stateSetGameMode();
  void stateSetLevelScreen();
  void stateSetLevelLevel();
  void stateDrawMpScreen();
  void stateRanking();
  void stateSetMpSettings();
  void stateStartGame();
  void stateNewBlock();
  void stateBlockDown();
  void stateMoveBlock(uint8_t ButtonActive);
  void stateRotateBlock();
  void stateFixBlock();
  void stateKillLine();
  void stateSpawnBlock();
  void stateUpdateScreen();
  void stateWaitOnStart();
  void stateWaitOnStartScreen();
  void stateInitializeCom();
  void stateGameOver();
  // transition requirements from states
  void changeStateInBlockDown();
  void changeStateIdle();

  void finalizeGame();
  void holdButtons();

  // Test function
  void testFct();
};

#endif /* SRCCPP_LOOPER_H_ */
