#ifdef ENABLE_NUNCHUCK
#include <Wire.h>

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 3, NEO_GRB + NEO_KHZ800);

// Doit etre ajuste en fonction de chaque nunchuck
#define ZEROX 530
#define ZEROY 530
#define ZEROZ 530

#define WII_NUNCHUK_I2C_ADDRESS 0x52	// adresse I2C du nunchuck
int chan;
uint8_t tpsc=0, tpsz=0;
uint8_t data[6];  //nunchuck
boolean c=false, z=false,	 cl=false, zl=false, op=false;

void nunchuck_init() {
	strip.begin();
		strip.setPixelColor(0, color[0], color[0+1], color[0+2]);
		strip.setBrightness(200);
		strip.show();
		
	Wire.begin();
	
	Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
	Wire.write(0xF0);
	Wire.write(0x55);
	Wire.endTransmission();
	
	Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
	Wire.write(0xFB);
	Wire.write(0x00);
	Wire.endTransmission();
	nunchuck_read();
	nunchuck_read();
}

void nunchuck_read() {
	// on demande 6 octets au nunchuck
	Wire.requestFrom(WII_NUNCHUK_I2C_ADDRESS, 6);

	chan = 0;
	// tant qu'il y a des donnees
	while(Wire.available()) { data[chan++] = Wire.read(); } // on recupere les donnees

	// on reinitialise le nunchuck pour la prochaine demande
	Wire.beginTransmission(WII_NUNCHUK_I2C_ADDRESS);
	Wire.write(0x00);
	Wire.endTransmission();
}
void nunchuck_update() {
	nunchuck_read();
	if(chan >= 5) {
		// on extrait les donnees
		// dans mon exemple j'utilise uniquement les donnees d'acceleration sur l'axe Y
		int JX = data[0];
		int JY = data[1];
		// on limite la valeur entre -180 et 180 puis reechantillonnage de 0-255
		double accelX = constrain(	((data[2] << 2) + ((data[5] >> 2) & 0x03) - ZEROX),		-180, 180);	//droite / gauche
		double accelY = constrain(	((data[3] << 2) + ((data[5] >> 4) & 0x03) - ZEROY),		-180, 180);	// av / ar
		double accelZ = constrain(	((data[4] << 2) + ((data[5] >> 6) & 0x03) - ZEROZ),		-180, 180);	// haut / bas
		
		// calcul appuis long sur boutton
		if((data[5] >> 0) & 1) { tpsc++; }	// compte temps
		else if(tpsc>300) { cl=!cl;	tpsc=0; }	// RAZ tps + déclaration appuis LONG
		else if(tpsc) { c=!c;	tpsc=0; }	// RAZ tps + déclaration appuis COURT
//		boolean zButton = !((data[5] >> 0) & 1);
		
		if((data[5] >> 1) & 1) { tpsz++; } else if(tpsz>300) { zl=!zl;	tpsz=0; } else if(tpsz) { z=!z;	tpsz=0; }
//		boolean cButton = !((data[5] >> 1) & 1);
		
		Servo_data[THROTTLE]	=	JY;
		Servo_data[ELEVATOR]	=	accelY;
		Servo_data[RUDDER]		=	accelX;
		Servo_data[AILERON]		=	JX;
		if(!op) {
			Servo_data[AUX1]		=	z;
			Servo_data[AUX2]		=	c;
			Servo_data[AUX3]		=	cl;
		}
	}
}

/*
#define DIVISOR_PERCENTS (32)
#define PERCENT_TO_BYTE(P) ((int8_t)((((int)P) * DIVISOR_PERCENTS) / 100))
#define BYTE_TO_PERCENT(P) ((int8_t)((((int)P) * 100) / DIVISOR_PERCENTS))
int16_t expo(int8_t a, int32_t x) {
  return (((BYTE_TO_PERCENT(a) * x  * x) / 100) * x) / (((int32_t)MAX_LEVEL) * MAX_LEVEL)
    + (100 - BYTE_TO_PERCENT(a)) * x / 100;
}
*/
#endif