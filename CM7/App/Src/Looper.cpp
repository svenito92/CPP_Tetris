/*
 * Looper.cpp
 *
 *  Created on: May 29, 2022
 *      Author: diktux
 */

#include "Looper.h"

// Constructor
Looper::Looper() {

	// TODO Auto-generated constructor stub
}

Looper::~Looper() {
	// TODO Auto-generated destructor stub
}

void Looper::run() {
	//HAL_UART_Transmit(&huart3,(const uint8_t*)"Start run\n", 10, 0xFFFF);

	// init system, ethernet, screen, buttons
	initScreen();
//	intercom_data_t *test;
	// main loop here
	processState = init;
	while (true) {
		buttons = ss.readButtons();
		switch (processState)
		 {
		 case init:
			 // Status LED
			 HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
			 HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
			 processState = selectGameModeSetScreen;
			// test->cmd = MQTT_RECEIVE;
			// sprintf((char*)&test->topic,"test");
			// mqtt_intercom__receive_cb(test);
			 break;
		 case selectGameModeSetScreen:
			 stateSetGameMode();
		 case selectGameModeBtnIn:
			 if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_1))
			 {
				 btnReleased((uint32_t)TFTSHIELD_BUTTON_1);
				 processState = gameSettingsSpSetScreen;
			 }
			 else if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_3))
			 {
				btnReleased((uint32_t)TFTSHIELD_BUTTON_3);
				roleMenu = true;
				role = 0;
				processState = gameSettingsMpDrawScreen;
			 }
			 //HAL_Delay(500);
			 break;
		 case gameSettingsSpSetScreen:
			 stateSetLevelScreen();
			 processState = gameSettingsSPSetLevel;
			break;
		 case gameSettingsSPSetLevel:
			 stateSetLevelLevel();
			 break;
		 case gameSettingsMpDrawScreen:
		 			 stateDrawMpScreen();
		 			 processState = gameSettingsMp;
		 	break;
		 case gameSettingsMp:
			 stateSetMpSettings();
			 //HAL_UART_Transmit(&huart3,(const uint8_t*)"Settings MP\n", 12, 0xFFFF);
			 //implement see single player and add mp parameters
			 break;
		 case singlePlayer:
			 gameMode=1;
			 //HAL_UART_Transmit(&huart3,(const uint8_t*)"SP game\n", 8, 0xFFFF);
			 runGame(); // singlePlayer as parameter
			 break;
		 case initializeCom:
			 stateInitializeCom();
			 processState = waitOnStart;
			 break;
		 case waitOnStart:
			 stateWaitOnStart();

			 break;
		 case multiPlayer:
			 gameMode = 2;
			 //HAL_UART_Transmit(&huart3,(const uint8_t*)"MP game\n", 12, 0xFFFF);
			 // implement
			 runGame();
			 break;
		 case gameOver:
			 stateGameOver();
			 HAL_UART_Transmit(&huart3,(const uint8_t*)"Game Over\n", 10, 0xFFFF);
			 // show screen and wait a moment
			 /*writeState("GAME OVER", ST7735_BLUE);
			 HAL_Delay(3000);
			 processState = ranking;*/
			 break;
		 case gameWon:
			 HAL_UART_Transmit(&huart3,(const uint8_t*)"Game Won\n", 9, 0xFFFF);
			 writeState("YOU WON", ST7735_BLUE); // show screen
			 processState = ranking;
			 break;
		 case ranking:
			 HAL_UART_Transmit(&huart3,(const uint8_t*)"Ranking\n", 8, 0xFFFF);
			 // show screen and wait a moment
			 processState = init;
			 break;
		 case testMode:
			 break;
		 }
	}
}

// init screen
void Looper::initScreen() {
	// init display
	ss.begin();
	// Start set the backlight on
	ss.setBacklight(TFTSHIELD_BACKLIGHT_ON);
	// Reset the TFT

	ss.tftReset(0);
	HAL_Delay(100);
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
	ss.tftReset(1);
	HAL_Delay(7);
	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);

	// Initialize TFT
	ST7735_Init();
	ST7735_FillScreen(ST7735_BLACK);
	//  setUpField();
}

void Looper::runGame() {
//	gameRunning = true;
//	while (gameRunning) {
		switch (gameState) {
		case startGame:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Start Game\n", 11, 0xFFFF);
			stateStartGame();		// Spielfeld gezeichnet, Nächster Block definiert, Variablen zurückgesetzt
			gameState = idle;
			break;
		case generateNewBlock:
			//HAL_UART_Transmit(&huart3,(const uint8_t*)"Generate blocks\n", 16, 0xFFFF);
			stateNewBlock();
			gameState = spawnblock; // insert new Block => Checkt ob Block platziert werden kann
			break;
		case blockDown:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Block down\n", 12, 0xFFFF);
			stateBlockDown();
			gameState = idle;
			break;
		case moveBlock:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Move block\n", 11, 0xFFFF);
			gameState = idle;
			stateMoveBlock(buttonPressed);
			buttonPressed=0;
			break;
		case rotateBlock:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Rotate block\n", 12, 0xFFFF);
			stateRotateBlock();
			gameState = idle;
			break;
		case idle:
			changeStateIdle();
			break;
		case fixBlock:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"fix block\n", 11, 0xFFFF);
			stateFixBlock();
			// CHECK IN MULTIPLAYER MODE HOW MANY PLAYERS REMAIN
			if (false) {  // check in MP mode
				processState = gameWon;
				finalizeGame();
			}
			else
			{
				gameState = killLine;  // Change state
			}
			break;
		case killLine:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Kill line\n", 10, 0xFFFF);
			// check all lines & kill line and move Lines above
			stateKillLine();
			// change state
			/*if (playground.isOverflow()) {
				finalizeGame();
				processState = gameOver;
			} else */
			if(gameMode == 2 ){	// MULTI PLAYER INSERT NEW LINE
				gameState = insertLine;
			} else{
				gameState = generateNewBlock;
			}
			break;
		case insertLine:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Insert line\n", 12, 0xFFFF);
			// CHECK HOW MANY LINES

			playground.insertLine(calculations.getRdmSpaceInNewLine());
			if (playground.isOverflow()) {
				finalizeGame();
				processState = gameOver;
			} else {
				gameState = generateNewBlock;
			}
			break;
		case spawnblock:
			HAL_UART_Transmit(&huart3,(const uint8_t*)"Spawn block\n", 12, 0xFFFF);
			//Check if new Block can be spawn or if colision
			stateSpawnBlock();

			break;
		case updateScreen:
			//HAL_UART_Transmit(&huart3,(const uint8_t*)"Update screen\n", 14, 0xFFFF);
			//Check if new Block can be spawn or if colision
			stateUpdateScreen();

			break;

		}
//	}
}

// Generates 5 new block in the array with default origin
void Looper::generateBlocks() {
	for (uint8_t i = 0; i < 5; i++) {
		playBlocks[i].renewBlock(calculations.getRdmBlock());
	}
}

// Accelerate game after a killed line
void Looper::accelerateGame(){
	if(blockDownCnt > 200){
		blockDownCnt -= 50;
	}
	else if (blockDownCnt > 100){
		blockDownCnt -= 10;
	}
	else if(blockDownCnt > 50){
		blockDownCnt -= 5;
	}
	else if(blockDownCnt > 10){
		blockDownCnt -= 1;
	}
	else{

	}
}

// Loop in this method until buttons are released
void Looper::btnReleased(uint32_t pressedButton) {
	while ((buttons & pressedButton) == 0) {
		// wait until button is released
		buttons = ss.readButtons();
	}
	HAL_UART_Transmit(&huart3,(const uint8_t*)"Btn released\n", 13, 0xFFFF);
}

/*
 * Action in states
 */
//
void Looper::stateSetGameMode(){
	HAL_UART_Transmit(&huart3, (const uint8_t*) "Select GameMode\n", 16, 0xFFFF);
	ST7735_FillScreen(ST7735_BLACK);
	writeTopLine("Single Pl", ST7735_BLUE);			// Show screen
	writeState("Multi Game", ST7735_BLUE);			// Show screen
	processState = selectGameModeBtnIn;
}

//
void Looper::stateSetLevelScreen(){
	HAL_UART_Transmit(&huart3, (const uint8_t*) "Set level SP\n", 13, 0xFFFF);
	ST7735_FillScreen(ST7735_BLACK);
	writeTopLine("Start", ST7735_CYAN);
	writeBtnMiddleLine("Level +", ST7735_RED);
	writeState("Level -", ST7735_GREEN);
	writeFourthLine(blockDownCnt, ST7735_BLUE);

}

//
void Looper::stateSetLevelLevel(){
	//HAL_UART_Transmit(&huart3, (const uint8_t*) "Set level SP2\n", 14, 0xFFFF);
	writeFourthLine(blockDownCnt, ST7735_BLUE);
	//HAL_Delay(500);

	if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_1)){
		processState = singlePlayer;
		btnReleased((uint32_t)TFTSHIELD_BUTTON_1);
	}
	else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_2)){
		if(blockDownCnt > 200){
			blockDownCnt -= 50;
			btnReleased((uint32_t)TFTSHIELD_BUTTON_2);
		}
		else{

		}
	}
	else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_3)){
		if(blockDownCnt < 3000){
			blockDownCnt += 50;
			btnReleased((uint32_t) TFTSHIELD_BUTTON_3);
		}
		else{

		}
	}
}

void Looper::stateDrawMpScreen(){
	if(roleMenu){
		ST7735_FillScreen(ST7735_BLACK);
		writeTopLine("A: Host", ST7735_CYAN);
		writeBtnMiddleLine("B: Player", ST7735_RED);
		HAL_Delay(300);
	}
	else{
		char StringPlNr[3] = {0, 0, 0};
//		ST7735_FillScreen(ST7735_BLACK);
		writeTopLine("Set Pl Nr", ST7735_CYAN);
		sprintf(StringPlNr, "%d", playerNr);
		ST7735_FillRectangle(0x0004, 0x0022, 50, 16, ST7735_BLACK);
		writeBtnMiddleLine(StringPlNr, ST7735_RED);
		HAL_Delay(300);
	}
}

//Sets Multiplayer Settings
void Looper::stateSetMpSettings(){
	 if(roleMenu){
		 if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_1))
		 { // button pushed
			btnReleased((uint32_t)TFTSHIELD_BUTTON_1);
			roleMenu = false;
			role = 1;
			playerNr=0;
			ST7735_FillScreen(ST7735_BLACK);
			processState = gameSettingsMpDrawScreen;
		 }
		 else if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_2))
		 {
			btnReleased((uint32_t)TFTSHIELD_BUTTON_2);
			roleMenu = false;
			role = 2;
			ST7735_FillScreen(ST7735_BLACK);
			processState = gameSettingsMpDrawScreen;
		 }
	 }
	 else{
		 if (!(buttons & (uint32_t) TFTSHIELD_BUTTON_LEFT))
		 { // button pushed
			//btnReleased((uint32_t)TFTSHIELD_BUTTON_1);
			if(playerNr <255){
				playerNr++;
			}
			processState = gameSettingsMpDrawScreen;
			HAL_Delay(200);
		 }
		 else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_RIGHT)){
			if(playerNr > 1){									// Player 0  is Master
				playerNr--;
			}
			processState = gameSettingsMpDrawScreen;
			HAL_Delay(200);
		 }
		 else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_DOWN)){
			if(playerNr > 10){									// Player 0  is Master
				playerNr=playerNr-10;
			}
			processState = gameSettingsMpDrawScreen;
			HAL_Delay(200);
		 }
		 else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_UP)){
			if(playerNr < 246){									// Player 0  is Master
				playerNr=playerNr+10;
			}
			processState = gameSettingsMpDrawScreen;
			HAL_Delay(200);
		 }
		 else if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_1)){
			 processState = initializeCom;
			HAL_Delay(200);
		 }
	 }
}

// TO DO, SET LEVEL, MULTIPLAYER
void Looper::stateStartGame() {
	blockDownCnt = INIT_BLOCK_DOWN_CNT;
	score = 0;
	killedLines = 0;
	blocksInGame = 0;
	for(uint8_t i = 0; i<=7;i++){
		blocksPerTypeInGame[i] = 0;
	}
	srand(time(0));
	generateBlocks(); // Hier gibt es bei mir einen HARDFAULT?? (Sven)
	setUpField(); 	// TFT fct
	writeScore(0x00000000, ST7735_BLUE); // set score to zero
	setPreview(playBlocks[currentBlockNo].getBlockType());
	playground.rstPlayground();
	/// set level
	// multiplayer settings
}

// increase the "pointer" with the current and next block or goes to zero
void Looper::stateNewBlock() {
	blocksInGame++;
	uint8_t tmpBlockType = playBlocks[currentBlockNo].getBlockType();
	blocksPerTypeInGame[tmpBlockType]++;		// cnt up per block type
	playBlocks[currentBlockNo].renewBlock(calculations.getRdmBlock());
	currentBlockNo++;
	if (currentBlockNo >= 5) {
		currentBlockNo = 0;
	}
	nextBlockNo++;
	if (nextBlockNo >= 5) {
		nextBlockNo = 0;
	}
}

//
void Looper::stateBlockDown() {
	// is alredy checked that the block is not on bottom

	playBlocks[currentBlockNo].moveOneLineDown();
	gameState = idle;
}

//
void Looper::stateMoveBlock(uint8_t ButtonActive) {
	// move possible
	// do move
	if(ButtonActive==1){
		if (playground.isSpaceRight(playBlocks[currentBlockNo].getBlockPositions())) {										// Joystick right
			// move right
			playBlocks[currentBlockNo].moveRight();
		}
	} else if(ButtonActive==2){
		if (playground.isSpaceLeft(playBlocks[currentBlockNo].getBlockPositions())) {										// Joystick left

		// move left
		playBlocks[currentBlockNo].moveLeft();
		}
	} else if (ButtonActive==3) {        //TO DO MOVE TO BOTTOM WHEN BUTTON PUSHED 	// Joystick down
		// Move to bottom
		// GET COLUMNS
		//void moveToBottom(uint8_t *fourColums);
		if (playground.isOnBottom(playBlocks[currentBlockNo].getBlockPositions())) {
					gameState = fixBlock;
		} else {                           // block down
					gameState = blockDown;
		}
	}
}

// state rotate block
void Looper::stateRotateBlock() {
	uint8_t rotatedPositions[4];
	playBlocks[currentBlockNo].getBlockRotatedPositions(&rotatedPositions[0]);
	if (playground.canRotate(&rotatedPositions[0])) {
		playBlocks[currentBlockNo].rotate();
	} else {
		;
	}
}

// state fix block
void Looper::stateFixBlock() {
	uint8_t *pointerBlockPos = playBlocks[currentBlockNo].getBlockPositions();
	for (uint8_t i = 0; i < 4; i++) {
		uint8_t fieldNo;
		fieldNo = *pointerBlockPos;
		playground.setField(fieldNo, playBlocks[currentBlockNo].getBlockType());
		pointerBlockPos++;
	}
}

// state killLine
void Looper::stateKillLine() {
	uint16_t oldKilledLines = killedLines;
	uint8_t linesSend=0;

	for (uint8_t line = 0; line < 21; line++) {
		if (playground.isLineFull(line)) {
			playground.killLine(line);
			score += 10000 / blockDownCnt;
			accelerateGame();
			killedLines++;
			writeScore(score, ST7735_BLUE);
		}
	}
	oldKilledLines= killedLines-oldKilledLines+openKilledLines;
	while(oldKilledLines!= 0){
		if(oldKilledLines > activePlayers){
			linesSend++;
			oldKilledLines - activePlayers;
		}
		else{
			openKilledLines = oldKilledLines;
			oldKilledLines=0;
		}
	}

	data->cmd = MQTT_PUBLISH;
	sprintf((char*)&data->topic,"KillLine");
	data->data_length = 2;
	data->data[0] = playerNr;
	data->data[1] = linesSend;
	mqtt_intercom__send(data);
	HAL_UART_Transmit(&huart3,(const uint8_t*)score +'0', 10, 0xFFFF);
}

// state spawnblock
void Looper::stateSpawnBlock() {
	uint8_t *pointerBlockPos = playBlocks[currentBlockNo].getBlockPositions();

	for (uint8_t i = 0; i < 4; i++) {
		if(playground.getField((*pointerBlockPos))!= 0)
		{
			processState = gameOver;
			gameState = startGame;
			i=10;
		}
		else
		{
			gameState = idle;
		}
		pointerBlockPos++;
	}
	return;
}

// change state in blockDown state
void Looper::changeStateIdle() {


	if ((HAL_GetTick()-timer) >= moveBlockTimer) {
		timer = HAL_GetTick();
		holdButtons();


//		LCD_CS0;
		counter++;
//		LCD_CS1;
		if ((((buttons&TFTSHIELD_BUTTON_UP)==0)&& (buttonUpHold==0))||(((buttons&TFTSHIELD_BUTTON_RIGHT)==0)&& (buttonRightHold==0))||((((buttons&TFTSHIELD_BUTTON_DOWN)==0))&& (buttonDownHold==0))) {                //BUTTON          //move block Joystick
			if((buttons&TFTSHIELD_BUTTON_UP)==0 && buttonUpHold==0){
				buttonPressed=1;
				buttonUpHold = 1;
			}
			else if(((buttons&TFTSHIELD_BUTTON_DOWN)==0)&& buttonDownHold==0) {
				buttonPressed=2;
				buttonDownHold = 1;
			}
			else if(((buttons&TFTSHIELD_BUTTON_RIGHT)==0)&& buttonRightHold==0) {
				buttonPressed=3;
				buttonRightHold = 1;
			}
			gameState = moveBlock;
		} else if((buttons&TFTSHIELD_BUTTON_1)==0 && buttonAHold==0) {    //BUTTON     		// rotate block Button A
			buttonAHold = 1;
			gameState = rotateBlock;
		} else if((buttons&TFTSHIELD_BUTTON_3)==0 && buttonCHold==0){							// fix block Button C
			buttonCHold = 1;
			if (playground.isOnBottom(playBlocks[currentBlockNo].getBlockPositions())){
				gameState = fixBlock;
			}
			else{
				while(!(playground.isOnBottom(playBlocks[currentBlockNo].getBlockPositions()))){
				stateBlockDown();
			}
				btnReleased((uint32_t) TFTSHIELD_BUTTON_3);
				gameState = fixBlock;
			}
		} else {
			;
		}
	}else if ((HAL_GetTick()-counter) >= blockDownCnt) {
		counter = HAL_GetTick();
		// fix block
		if (playground.isOnBottom(playBlocks[currentBlockNo].getBlockPositions())) {
			gameState = fixBlock;
		} else {                           // block down
			gameState = blockDown;
		}
	} else if ((HAL_GetTick()-updateScreenCounter)>=updateScreenTime) {                              // stay in state
		gameState = updateScreen;
	}else
	{
		;
	}
}

// update Screen in Blocking State
void Looper::stateUpdateScreen() {

	uint8_t unitedFieldData[200]={0};

	for(uint8_t i=0;i<=199; i++)
	{
		unitedFieldData[i]= playground.getField(i+10) ;
	}

	uint8_t *pointerBlockPos = playBlocks[currentBlockNo].getBlockPositions();
	uint8_t fieldNo;

	for (uint8_t i = 0; i < 4; i++) {
		fieldNo = *pointerBlockPos-10;
		unitedFieldData[fieldNo] = playBlocks[currentBlockNo].getBlockType();
		pointerBlockPos++;
	}
	drawField(unitedFieldData);
	setPreview(playBlocks[nextBlockNo].getBlockType());
	gameState = idle;
}

void Looper::stateWaitOnStart(){
	intercom_data_t *data
	if(!(buttons & (uint32_t) TFTSHIELD_BUTTON_3)){
		processState = 	gameSettingsMpDrawScreen;

	}
	else if(gameStartFlag != 0){
		processState = multiPlayer;

	}
	else{
		processState = 	waitOnStart;
		if(HAL_GetTick()-playerIdUpdate >= 500) {
			playerIdUpdate = HAL_GetTick();
			data->cmd = MQTT_PUBLISH;
			sprintf((char*)&data->topic,"Players");
			data->data_length = 1;
			data->data = playerNr;
			mqtt_intercom__send(data);
		}
	}
}

void Looper::stateInitializeCom(){
intercom_data_t *data;

	if(interCoreComReady = true){
		data->cmd = MQTT_SUBSCRIBE;
		sprintf(data->topic,"StartGame");
		mqtt_intercom__send(data);
		data->cmd = MQTT_SUBSCRIBE;
		sprintf(data->topic,"Players");
		mqtt_intercom__send(data);
		data->cmd = MQTT_SUBSCRIBE;
		sprintf(data->topic,"KillLine");
		mqtt_intercom__send(data);
		data->cmd = MQTT_SUBSCRIBE;
		sprintf(data->topic,"GameOver");
		mqtt_intercom__send(data);
		data->cmd = MQTT_SUBSCRIBE;
		sprintf(data->topic,"GameWon");
		mqtt_intercom__send(data);
	}
}

void stateGameOver(){

	 processState = ranking;


}
// Finalize game, change states and stop loop
void Looper::finalizeGame() {
	gameState = startGame;
}

// Test function
void Looper::testFct() {
	bool runTest = true;
	while (runTest) {
		// PUSH SPARE BUTTON TO GO TO THE NEXT TEST
		// SHOW RESULTS ON SCREEN
		for (uint8_t i = 0; i < 210; i++) {
			test.createTestPlayground(i);
		}
		// PUSH BUTTON
		stateKillLine();

		// PUSH BUTTON
		playground.insertLine(3);


		for (uint8_t i = 0; i < 210; i++) {
			test.createGapSidePlaygrount(i);
		}

	}
}

void Looper::holdButtons()
{

	if(buttonAHold!=0){
		buttonAHold++;
		if (buttonAHold>=3) buttonAHold=0;
	}

	if(buttonCHold!=0){
		buttonCHold++;
		if (buttonCHold>=5) buttonCHold=0;
	}

	if(buttonUpHold!=0){
		buttonUpHold++;
		if (buttonUpHold>=3) buttonUpHold=0;
	}

	if(buttonDownHold!=0){
		buttonDownHold++;
		if (buttonDownHold>=3) buttonDownHold=0;
	}

	if(buttonRightHold!=0){
		buttonRightHold++;
		if (buttonRightHold>=3) buttonRightHold=0;
	}
}
