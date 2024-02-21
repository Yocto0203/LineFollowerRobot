#include <Servo.h>

//左側のモーター
#define IN1 22
#define IN2 23
//右側のモーター
#define IN3 24
#define IN4 25
//上のモータ
#define IN3_2 3
#define IN4_2 4
#define ENB_2 5

#define ENA 9
#define ENB 10

//ボタン、ブザー
#define BUTTONA 12 
#define buzzPin 2

//ラインセンサーの入力
#define R_SENC 1
#define MID_SENC 2
#define L_SENC  3
//1 black
int SPEED = 0;
int L_SENCE_TH = 100;
int M_SENCE_TH = 100;
int R_SENCE_TH = 100;

int R_TH = 180;
int L_TH = 180;

void R_MOTOR_CONTROL(int power, bool brak=false, bool reversal=false);
void L_MOTOR_CONTROL(int power, bool brak=false, bool reversal=false);
void TOP_MOTOR_CONTROL(int power, int duration, bool brak=false, bool reversal=false);
int LINE_TRACE(int r_offset=0, int l_offset=0);
bool IS_ON_LINE(int SENCE_PIN);
void MOVE_TO(int L_MOTOR_POW, int R_MOTOR_POW, int duration, bool brak=false);
void Adjustment();
void debug();
int GET_LINE_POS();
Servo myservo; 

void setup() {
  // put your setup code here, to run once:
  pinMode(IN1, OUTPUT);  // デジタルピンを出力に設定
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(BUTTONA, INPUT_PULLUP);
  Serial.begin(9600);
  myservo.attach(13);
}



int counter = 0;
int prev_state = -1;
int first_time_flag = true;
int flag_2 = true;
int speed_up_flag = true;

int black_line_check = false;
unsigned long world_t = 0;

double MOVE_TO_DIV_VAL = 3.3;

int only_onece_one = true;
int only_onece_two = true;
int only_onece_three = true;

int black_block = true;

void loop() {

  //閾値調整
  if(first_time_flag == true){
      myservo.write(0);
      //センサーしきい値調整
      Adjustment();

      // スタートボタンが押されるまで待機
      while(digitalRead(BUTTONA) == HIGH) {
        debug();
      }
      myservo.write(180);
      tone(buzzPin,2637,1000);
      delay(1000);

      //アーム開放
      TOP_MOTOR_CONTROL(255, 700, false, true);

      //フラグOFF
      first_time_flag =false;
      world_t = millis();
    }
  

  //スピードを徐々に上げる
  if (speed_up_flag == true){
    SPEED += 1;
    if (SPEED >= 255){
      speed_up_flag = false;
    }
  }

  //ライントレース
  int state = LINE_TRACE(0, 0);

  //黒ライン検知を許可するか?
  if(black_line_check == false){
    if (black_block == true){//最初は検知しない
      if (millis() - world_t > 10*1000){
        black_line_check = true;
        black_block = false;
      }
    }else{
      if (millis() - world_t > 800){
        black_line_check = true;
      }
    }
  }
  
  //黒のライン検知
  if(state == 0 and prev_state != 0 and black_line_check == true){
    if (GET_LINE_POS() == 0){
      counter += 1;
      black_line_check = false;
      world_t = millis();
      tone(buzzPin,1318,500);
    }
  }

  //1つ目の黒黒黒を踏んだ時
  if(state == 0 and counter == 1 and only_onece_one == true){
    SPEED = 100;
    only_onece_one = false;
  }
  
  //2つ目の黒黒黒を踏んだ時
  if(state == 0 and counter == 2 and only_onece_two == true){
    only_onece_two = false;
    SPEED = 180;
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    TOP_MOTOR_CONTROL(255, 1000);
    delay(500);
    
    int back_to_line_check_counter = 0;
    while(back_to_line_check_counter < 3){
      L_MOTOR_CONTROL(SPEED/2);
      R_MOTOR_CONTROL(0);
      //ラインに戻ってきたかのチェック
      if (GET_LINE_POS() == 1 or GET_LINE_POS() == 5){
        back_to_line_check_counter++;
      }else{
        back_to_line_check_counter=0;
      }
    }
    SPEED = 80;
  }



  if (state == 0 and counter == 3 and only_onece_three == true){
    SPEED = 100;
    only_onece_three = false;
  }

  if (state == 0 and counter == 4){
    MOVE_TO(SPEED, SPEED, 50, true);
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    //ちょっと下げる
    
    unsigned long t1 = millis();
    delay(500);

    //アーム下ろす
    SPEED = 128;
    TOP_MOTOR_CONTROL(255, 1000, false, true);
    t1 = millis();
    while(millis() - t1 < 1300){
      //L_MOTOR_CONTROL(180,false, true);
      //R_MOTOR_CONTROL(80, false, true);
      
      if(GET_LINE_POS() == 5){//白黒白
        L_MOTOR_CONTROL(SPEED, false, true);
        R_MOTOR_CONTROL(SPEED, false, true);
        
      }else if(GET_LINE_POS() == 2){//黒白白
        L_MOTOR_CONTROL(SPEED, false, true);
        R_MOTOR_CONTROL(SPEED/2, false, true);
        
      }else if(GET_LINE_POS() == 6){//白白白
        L_MOTOR_CONTROL(SPEED,false, true);
        R_MOTOR_CONTROL(SPEED/4, false, true);
      }else{
        L_MOTOR_CONTROL(SPEED, false, true);
        R_MOTOR_CONTROL(SPEED, false, true);
      }
      
    }

    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    delay(3000);

    SPEED = 128;
  }

  //橋を渡ってる間
  if (state == 0 and counter == 5){
    int s = -1;
    unsigned long t = millis();
    while(millis() - t < 2500){
      LINE_TRACE(0, 0);
    }
  }

  if (state == 0 and counter == 7){
    //黒々黒の間進み続ける
    while(LINE_TRACE(0, 0) == 0);
    //止まる
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    MOVE_TO(255, 100, 1000, true);
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    MOVE_TO(128, 128, 500, true);
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);

    myservo.write(0);
    MOVE_TO(255, 255, 500, true);
    L_MOTOR_CONTROL(0, true);
    R_MOTOR_CONTROL(0, true);
    delay(100);
    
    for(int i = 0; i < counter; i++){
      tone(buzzPin,262,500) ;
      delay(1000);
    }
    while(digitalRead(BUTTONA) == HIGH){
      debug();
    }
  }
/*
    Serial.print("three");
    Serial.print(",");
    Serial.print("two");
    Serial.print(",");
    Serial.println("one");
    Serial.print(analogRead(3));
    Serial.print(",");
    Serial.print(analogRead(2));
    Serial.print(",");
    Serial.println(analogRead(1));
*/
    debug();
    prev_state = state;
    delay(5);
}

void MOVE_TO(int L_MOTOR_POW, int R_MOTOR_POW, int duration, bool brak){
  unsigned long t1 = millis();
  while(millis() - t1 < duration){
    L_MOTOR_CONTROL(L_MOTOR_POW);
    R_MOTOR_CONTROL(R_MOTOR_POW);
  }
    L_MOTOR_CONTROL(0, brak);
    R_MOTOR_CONTROL(0, brak);
}

void L_MOTOR_CONTROL(int power, bool brak, bool reversal){
  //power 0~255まで、出力設定
  //brak trueでブレーキをかける
  //reversal trueで逆転
  if (brak == true){
    digitalWrite(IN1, HIGH); // 両端子HIGHでブレーキ
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, 255);
  }else if(reversal == true){
    digitalWrite(IN1, LOW);  // HIGH LOWの組み合わせでモーター回転
    digitalWrite(IN2, HIGH); // 逆回転
    analogWrite(ENA, power);
  }else{
    digitalWrite(IN1, HIGH); //正転
    digitalWrite(IN2, LOW);
    analogWrite(ENA, power);
  }
}

void R_MOTOR_CONTROL(int power, bool brak, bool reversal){
  if (brak == true){
    digitalWrite(IN3, HIGH); // 両端子HIGHでブレーキ
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, 255);
  }else if(reversal == true){
    digitalWrite(IN3, LOW);  // HIGH LOWの組み合わせでモーター回転
    digitalWrite(IN4, HIGH); // 逆回転
    analogWrite(ENB, power);
    //AnalogWrite_EN(ENB, power, duration, off_duration, &ENB_time, im);
  }else{
    digitalWrite(IN3, HIGH); //正転
    digitalWrite(IN4, LOW);
    analogWrite(ENB, power);
    //AnalogWrite_EN(ENB, power, duration, off_duration, &ENB_time, im);
    
  }
}

void TOP_MOTOR_CONTROL(int power, int duration, bool brak, bool reversal){
  unsigned long t1 = millis();
  while(millis() - t1 < duration){
    if (brak == true){
      digitalWrite(IN3_2, HIGH); // 両端子HIGHでブレーキ
      digitalWrite(IN4_2, HIGH);
      analogWrite(ENB_2, 255);
    }else if(reversal == true){
      digitalWrite(IN3_2, HIGH);  // HIGH LOWの組み合わせでモーター回転
      digitalWrite(IN4_2, LOW); // 逆回転
      analogWrite(ENB_2, power);
      //AnalogWrite_EN(ENB, power, duration, off_duration, &ENB_time, im);
    }else{
      digitalWrite(IN3_2, LOW); //正転
      digitalWrite(IN4_2, HIGH);
      analogWrite(ENB_2, power);
      //AnalogWrite_EN(ENB, power, duration, off_duration, &ENB_time, im);
      
    }
  }
  digitalWrite(IN3_2, HIGH); // 両端子HIGHでブレーキ
  digitalWrite(IN4_2, HIGH);
  analogWrite(ENB_2, 255);
}


bool IS_ON_LINE(int SENCE_PIN){
  //線上ならtrue, それ以外ならfalse
  int TH = 0;
  
  if(SENCE_PIN == L_SENC){
    TH = L_SENCE_TH;
  }else if(SENCE_PIN == MID_SENC){
    TH = M_SENCE_TH;
  }else if(SENCE_PIN == R_SENC){
    TH = R_SENCE_TH;
  }
  
  if (analogRead(SENCE_PIN) >= TH){
    return true; //黒
  }else{
    return false;
  }
}

int GET_LINE_POS(){
  int state = -1;
  if(IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      state = 0; //黒黒黒
  }else if (IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      state = 1; //黒黒白
  }else if (IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      state = 2; //黒白白
  }else if (!IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      state = 3; //白黒黒
  }else if (!IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      state = 4; //白白黒
  }else if (!IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      state = 5; //白黒白
  }else if (!IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      state = 6; //白白白
  }

  return state;
}


int LINE_TRACE(int r_offset, int l_offset){
  int state = -1;
  if(IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      //R_MOTOR_CONTROL(0, true);
      //L_MOTOR_CONTROL(0, true);
      R_MOTOR_CONTROL(SPEED + r_offset, false);
      L_MOTOR_CONTROL(SPEED + l_offset, false);
      state = 0; //黒黒黒
  }else if (IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL(SPEED + r_offset, false);
      L_MOTOR_CONTROL((SPEED + l_offset)/2, false);
      state = 1; //黒黒白
  }else if (IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL(SPEED + r_offset, false, false);
      L_MOTOR_CONTROL(0, false);
      state = 2; //黒白白
  }else if (!IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL((SPEED + r_offset)/2, false);
      L_MOTOR_CONTROL(SPEED + l_offset, false);
      state = 3; //白黒黒
  }else if (!IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL(0, false);
      L_MOTOR_CONTROL(SPEED + l_offset, false, false);
      state = 4; //白白黒
  }else if (!IS_ON_LINE(L_SENC) and IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL(SPEED + r_offset, false);
      L_MOTOR_CONTROL(SPEED + l_offset, false);
      state = 5; //白黒白
  }else if (!IS_ON_LINE(L_SENC) and !IS_ON_LINE(MID_SENC) and !IS_ON_LINE(R_SENC)){
      R_MOTOR_CONTROL(SPEED + r_offset, false);
      L_MOTOR_CONTROL(SPEED + l_offset, false);
      state = 6; //白白白
  }

  return state;
}

void Adjustment(){
  ADJAST_FIRST:
  //前のセンサー
  int L1=0;
  int M1=0;
  int R1=0;
  int L2=0;
  int M2=0;
  int R2=0;

  
  while(digitalRead(BUTTONA) == HIGH) ; //ボタンを押すまで無限ループ
  tone(buzzPin,262,100);
  delay(500);

  //各センサーの平均 - 1回目
  for(int i=0; i < 10; i++){
    L1 += analogRead(L_SENC);
    M1 += analogRead(MID_SENC);
    R1 += analogRead(R_SENC);
  }
  L1 = L1/10;
  M1 = M1/10;
  R1 = R1/10;

  
  tone(buzzPin,262,100);
  delay(100);
  tone(buzzPin,294,100);
  delay(100);
  tone(buzzPin,330,100);
  delay(100);

  while(digitalRead(BUTTONA) == HIGH) ; //ボタンを押すまで無限ループ
  tone(buzzPin,262,100);
  delay(500);

  //各センサーの平均 - 2回目
  for(int i=0; i < 10; i++){
    L2 += analogRead(L_SENC);
    M2 += analogRead(MID_SENC);
    R2 += analogRead(R_SENC);
  }
  L2 = L2/10;
  M2 = M2/10;
  R2 = R2/10;

  //閾値が適正かチェック
  int MIN_RANG = 70; //最低限、これだけ差がないといけない
  if(abs(L2-L1) < MIN_RANG or abs(M2-M1) < MIN_RANG or abs(R2-R1) < MIN_RANG){
     tone(buzzPin,262,200);
     delay(250);
     tone(buzzPin,262,200);
     delay(250);
     tone(buzzPin,262,200);
     delay(250);
     goto ADJAST_FIRST;
  }

  L_SENCE_TH = (L2+L1)/2;
  M_SENCE_TH = (M2+M1)/2;
  R_SENCE_TH = (R2+R1)/2;

  tone(buzzPin,392,100);
  delay(100);
  tone(buzzPin,440,100);
  delay(100);
  
}


void debug(){
  if(IS_ON_LINE(L_SENC)){
    digitalWrite(53,HIGH);
  }else{
    digitalWrite(53,LOW);
  }
   if(IS_ON_LINE(MID_SENC)){
    digitalWrite(51,HIGH);
  }else{
    digitalWrite(51,LOW);
  }
    if(IS_ON_LINE(R_SENC)){
    digitalWrite(49,HIGH);
  }else{
    digitalWrite(49,LOW);
  }
}
