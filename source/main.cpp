#include <nds.h>

#include <stdio.h>

#include <gl2d.h>

#include <fat.h>

//Global variables
u16 pressed;
s8 frameCounter;
u16 bigFoodFrameCounter;
u16 playerScore;
u16 highScore = 300;
u16 savedScore;
FILE* savefile;
bool startGame = false;
bool gameOver = false;
bool saveFileExists = false;
bool currentGameSaved = false;
bool buttonPressed = false;

//Function Declarations
void ShowMenu();
void DrawBox(s16 xPos, s16 yPos, u8 width, u8 height, u8 r, u8 g, u8 b);
void topScreenInfo(u16 score);
void SetDefaults();
void PlayGameLoop();
void SetHighScore();//Sets the current score to the highscore if the current highscore has been beaten
void LoadSaveFile();//Checks if a savefile exists and gets the data found in the savefile
void CreateNewSaveFile();//Creates a new save file if there is no save file yet (SaveFileExists == false)
void SaveHighScore();//Writes a new highscore to the savefile
bool IsCollided(s16 x1, s16 y1, s16 x2, s16 y2, s8 size1, s8 size2);//Checks collision of two boxes
u16 RandomU16(u16 minNo, u16 maxNo);//Generates a random u16 in a range

//Struct for save file
struct VariablesToSave {
u16 savedHighScore;
} SaveFile;

//Classes
class Snake{
    private:
        const static u8 initXPos = SCREEN_WIDTH/2;
        const static u8 initYPos = SCREEN_HEIGHT/2;
        const static u8 _DEFAULT_LENGTH = 4;
        const static u8 _MAX_LENGTH = 255;
    public:
        s16 xPos = initXPos;
        s16 yPos = initYPos;
        s8 xOffset = 0;
        s8 yOffset = 0;
        s8 xVelocity = 1;
        s8 yVelocity = 0;
        s8 boxSize = 8;
        u8 snakeLength;
        s16 xPosArr[_MAX_LENGTH];
        s16 xPosPrev[_MAX_LENGTH];
        s16 yPosArr[_MAX_LENGTH];
        s16 yPosPrev[_MAX_LENGTH];
        Snake(){
            snakeLength = _DEFAULT_LENGTH;
            for(u8 i = 0;i<snakeLength;i++){
                yPosPrev[i] = yPos;
                xPosPrev[i] = xPos - (boxSize * (1+i));
            }
        }
        void drawSnake(){
            glBegin2D();
            for(u8 i = 0;i<snakeLength;i++){
                if(i==0){
                    xPosArr[i] = xPos;
                    yPosArr[i] = yPos;
                    DrawBox(xPosArr[i],yPosArr[i],boxSize,boxSize,255,255,255);
                }
                else{
                    xPosArr[i] = xPosPrev[i-1];
                    yPosArr[i] = yPosPrev[i-1];
                    DrawBox(xPosArr[i],yPosArr[i],boxSize,boxSize,255,255,255);
                }
            }
            glEnd2D();
        }
        void CheckInputs(){
            if(KEY_UP & pressed && yVelocity == 0 && !buttonPressed){
                yVelocity = -1;
                xVelocity = 0;
                buttonPressed = true;
            }
            else if(KEY_DOWN & pressed && yVelocity == 0 && !buttonPressed){
                yVelocity = 1;
                xVelocity = 0;
                buttonPressed = true;
            }
            else if(KEY_LEFT & pressed && xVelocity == 0 && !buttonPressed){
                yVelocity = 0;
                xVelocity = -1;
                buttonPressed = true;
            }
            else if(KEY_RIGHT & pressed && xVelocity == 0 && !buttonPressed){
                yVelocity = 0;
                xVelocity = 1;
                buttonPressed = true;
        }
}
        void addBlock(){
            if(snakeLength<_MAX_LENGTH){
            snakeLength++;
            }
        }
        void updateSnake(){
            xPos+=xOffset;
            yPos+=yOffset;
            for(u8 i = 0;i<snakeLength;i++){
                xPosPrev[i] = xPosArr[i];
                yPosPrev[i] = yPosArr[i];
            }
        }
        bool IsHitSelf(){
            for(u8 i = 3;i<snakeLength;i++){
                if(xPosArr[0]==xPosArr[i]&&yPosArr[0]==yPosArr[i]){
                return true;
            }
        }

            return false;
        }
        void oneStepBack(){
            for(u8 i = 0;i<snakeLength;i++){
                xPosArr[i] = xPosPrev[i];
                yPosArr[i] = yPosPrev[i];
            }
        }
        void CheckScreenEdges(){
            if(xPos>SCREEN_WIDTH-1){
                xPos = 0;
            }
            else if(xPos<0){
                xPos = SCREEN_WIDTH-boxSize;
            }
            else if(yPos>SCREEN_HEIGHT-1){
                yPos = 0;
            }
            else if(yPos<0){
                yPos = SCREEN_HEIGHT-boxSize;
            }
        }
        void resetSnake(){
            snakeLength = _DEFAULT_LENGTH;
            xOffset = 0;
            yOffset = 0;
            xVelocity = 1;
            yVelocity = 0;
            xPos = initXPos;
            yPos = initYPos;
            for(u8 i = 0;i<snakeLength;i++){
                yPosPrev[i] = initYPos;
                xPosPrev[i] = initXPos - (boxSize * (1+i));
            }
        }
};
class Food{
private:
    const static u8 _DEFAULT_X = 20;
    const static u8 _DEFAULT_Y = 20;
    const static u8 _DEFAULT_R = 255;
    const static u8 _DEFAULT_G = 213;
    const static u8 _DEFAULT_B = 0;
public:
    u8 foodSize;
    u8 scoreAmount;
    s16 foodXPos;
    s16 foodYPos;
    Food(u8 inputSize, u8 inputScoreAmount){
        foodSize = inputSize;
        scoreAmount = inputScoreAmount;
        foodXPos = _DEFAULT_X;
        foodYPos = _DEFAULT_Y;
    }
    void drawFood(){
        glBegin2D();
        DrawBox(foodXPos,foodYPos,foodSize,foodSize,_DEFAULT_R,_DEFAULT_G,_DEFAULT_B);
        glEnd2D();
    }
    void drawFood(u8 r, u8 g, u8 b){
        glBegin2D();
        DrawBox(foodXPos,foodYPos,foodSize,foodSize,r,g,b);
        glEnd2D();
    }
    void setFoodPos(Snake snake){//generates a random position for the food and makes sure will not be overlapping with the snake, so a 'Snake' object is needed
        foodXPos = GenerateNewFoodXPos();
        foodYPos = GenerateNewFoodYPos();
        for(u8 i = 0; i<snake.snakeLength; i++){
            if(IsCollided(foodXPos, foodYPos, snake.xPosArr[i], snake.yPosArr[i], foodSize, snake.boxSize)){
                setFoodPos(snake);
            }
        }
    }

    void setFoodPos(s16 inputXPos, s16 inputYPos){
        foodXPos = inputXPos;
        foodYPos = inputYPos;
    }
    u16 GenerateNewFoodXPos(){
        return RandomU16(10,SCREEN_WIDTH-10);
    }
    u16 GenerateNewFoodYPos(){
        return RandomU16(10,SCREEN_HEIGHT-10);
    }
};

//Class instances
Snake playerSnake;
Food snakeFood(8,10);
Food bigSnakeFood(15,120);

//Main function
int main(void) {

    fatInitDefault();
    videoSetMode( MODE_5_3D );
    lcdMainOnBottom();
    glScreen2D();
    SetDefaults();
	consoleDemoInit();
	LoadSaveFile();
	if(!saveFileExists){
        CreateNewSaveFile();
	}
	while(1) {
        ShowMenu();
        pressed = keysDown();
        scanKeys();
        if(KEY_START & pressed||KEY_A & pressed){
            startGame = true;
            currentGameSaved = false;
        }
        if(gameOver && (KEY_START & keysDown()||KEY_A & keysDown())){
            SetDefaults();
		}
        if(startGame){
           PlayGameLoop();
        }
        SetHighScore();
		glFlush(0);
		swiWaitForVBlank();
	}
	return 0;
}

//Function Bodies
void DrawBox(s16 xPos, s16 yPos, u8 width, u8 height, u8 r, u8 g, u8 b){
    glBoxFilled(xPos+1,yPos+1,xPos+width-1,yPos+height-1,RGB15(r,g,b));
}
void topScreenInfo(u16 high, u16 player){
    consoleClear();
    iprintf("\n\n\tSNAKE DS\n");
    iprintf("\n\tScore: %d\n",player);
    iprintf("\n\tHigh score: %d\n",high);
    if(gameOver){
            iprintf("\n\tGAME OVER");
            iprintf("\n\n\n\tPress START to play again.");
            if(!currentGameSaved){
                SaveHighScore();
                currentGameSaved = true;
            }
        }
}
void SetDefaults(){
    frameCounter = 0;
    bigFoodFrameCounter = 0;
    playerScore = 0;
    bigSnakeFood.setFoodPos(300,300);
    snakeFood.setFoodPos(20,20);
    buttonPressed = false;
    playerSnake.resetSnake();
    gameOver = false;
}
void PlayGameLoop(){
        frameCounter++;
        bigFoodFrameCounter++;
        topScreenInfo(highScore,playerScore);
        playerSnake.CheckInputs();
        playerSnake.CheckScreenEdges();
        if(bigFoodFrameCounter==600&&!gameOver){
            bigSnakeFood.setFoodPos(playerSnake);
        }
        if(bigFoodFrameCounter==900&&!gameOver){
            bigFoodFrameCounter = 0;
            bigSnakeFood.setFoodPos(300,300);
        }
		if(frameCounter==6&&!gameOver){
            buttonPressed = false;
            playerSnake.xOffset = playerSnake.xVelocity*playerSnake.boxSize;
            playerSnake.yOffset = playerSnake.yVelocity*playerSnake.boxSize;

            if(IsCollided(playerSnake.xPos,playerSnake.yPos,snakeFood.foodXPos,snakeFood.foodYPos,playerSnake.boxSize,snakeFood.foodSize)){
                playerSnake.addBlock();
                playerScore+=snakeFood.scoreAmount;
                snakeFood.setFoodPos(playerSnake);
            }
            if(IsCollided(playerSnake.xPos,playerSnake.yPos,bigSnakeFood.foodXPos,bigSnakeFood.foodYPos,playerSnake.boxSize,bigSnakeFood.foodSize)){
                playerSnake.addBlock();
                playerScore+=bigSnakeFood.scoreAmount;
                bigSnakeFood.setFoodPos(300,300);
            }
            if(playerSnake.IsHitSelf()){
                playerSnake.xOffset = 0;
                playerSnake.yOffset = 0;
                playerSnake.oneStepBack();
                gameOver = true;
            }
            playerSnake.updateSnake();
            frameCounter = 0;
        }

		playerSnake.drawSnake();
		snakeFood.drawFood();
		if(bigFoodFrameCounter<850&&!gameOver){
		bigSnakeFood.drawFood(5,255,5);
		}
		else if(frameCounter<=2&&!gameOver){
		bigSnakeFood.drawFood(20,255,20);
		}

}
void ShowMenu(){
    consoleClear();
    iprintf("\n\n\tSNAKE DS");
    iprintf("\n\n\tHigh score: %d",highScore);
    iprintf("\n\n\n\n\tPress START\n");
}
void SetHighScore(){
    if(playerScore>highScore){
            highScore = playerScore;
		}
}
void CreateNewSaveFile(){

    SaveFile.savedHighScore = highScore;
    savefile = fopen("fat:/snakends.sav","wb");
    fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
    fclose(savefile);
}
void LoadSaveFile(){
    savefile = fopen("fat:/snakends.sav","rb");
	if(savefile != NULL){
        saveFileExists = true;
        fread(&SaveFile,1,sizeof(SaveFile),savefile);
        savedScore = SaveFile.savedHighScore;
        if(savedScore>highScore){
            highScore = savedScore;
        }
	}
	else{
        saveFileExists = false;
        savedScore = 0;
	}
	fclose(savefile);
}
void SaveHighScore(){
    if(highScore>savedScore){
    SaveFile.savedHighScore = highScore;
    savedScore = SaveFile.savedHighScore;
    savefile = fopen("fat:/snakends.sav","wb");
    fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
    fclose(savefile);
    }
}
bool IsCollided(s16 x1, s16 y1, s16 x2, s16 y2, s8 size1, s8 size2){
    if(((x1>=x2&&x1<=x2+size2)||(x1<=x2&&x1+size1>=x2))&&((y1>=y2&&y1<=y2+size2)||(y1<=y2&&y1+size1>=y2))){
        return true;
    }
    if(x1==x2&&y1==y2){
        return true;
    }
    return false;
}
u16 RandomU16(u16 minNo, u16 maxNo){
    return rand()%(maxNo-minNo+1)+minNo;
}







