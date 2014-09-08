add new lines here
here
here
here
here
here
here
here
herehere
here
here
herehere



byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] =  { 172,16,1,50 };

const int chipSelect = 4;

const int MAX_PAGENAME_LEN = 8; // max characters in page name
char buffer[MAX_PAGENAME_LEN+1]; // additional character for terminating null

EthernetServer server(8088);
EthernetClient client;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(54,86,132,254);  // numeric IP for data.sparkfun.com
char serverToSendTo[] = "data.sparkfun.com";    // name address for data.sparkFun (using DNS)

/////////////////////////
// Google Chart arrays //
/////////////////////////
const int chartArrayLength = 2;
int windSpeedArray[chartArrayLength];
int windDirectionArray[chartArrayLength];
int temperatureArray[chartArrayLength];
int humidityArray[chartArrayLength];

//

/////////////////
// Phant Stuff //
/////////////////
//const String publicKey = "MGGaNdLE49u1lp3Kd3V1";	// http://data.sparkfun.com/streams/MGGaNdLE49u1lp3Kd3V1
//const String privateKey = "nzzmlnXeDMCPZvVl1VjP";
const byte NUM_FIELDS				= 6;
const String fieldNames[NUM_FIELDS] = {"hotWater", "humidity", "temperature", "windDirection", "windSpeed", "RemoteIP"};
String fieldData[NUM_FIELDS];


const int hotWaterPin		= 13;

int currentTemp				= random(46);
int currentHumidity			= random(101);
int currentWindSpeed		= random(181);
int currentWindDirection	= random(360);
bool hotWaterPinState		= 0;
bool lastHotWaterState		= 1;		// holds last state for tweeting change of state  (its one so it will tweet on every boot as well)
byte rip[4];							// holds IP of connected PC
byte ripTweet[3];						// IP to tweet
String lastIP;

//int webPageLoops = 0;
int currentTempAverage, currentHumidityAverage, currentWindSpeedAverage, currentWindDirectionAverage;

unsigned long pageViews			= 1;
unsigned long lastUpdate;
unsigned long lastAutoUpdate;
unsigned long last404;
unsigned long lastIpLog;
unsigned long lastTweetAboutSdCard;
unsigned long sdErrorTweetInterval	= 3600000;	// 1 hour between failed SD card tweets
unsigned int autoUpdateInterval = 60000;	// wait xx ms between auto updates to any logging or tweeting functions


void setup()
{
	//wdt_disable();
	wdt_reset();

  Serial.begin(115200); //Start serial port for debugging
  Ethernet.begin(mac, ip);
  server.begin();

  pinMode(hotWaterPin,OUTPUT);

  // set array data to 0's
  for (int i = 0; i <= chartArrayLength; i++)
  {
	  windSpeedArray[i] = 0;
	  windDirectionArray[i] = 0;
	  temperatureArray[i] = 0;
	  humidityArray[i] = 0;
  }

  //	init SD card
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);		// make sure that the default chip select pin is set to output, even if you don't use it:

  if (!SD.begin(chipSelect))	// see if the card is present and can be initialized:
  {
	  Serial.println("Card failed, or not present");

	  delay(random(598));

		char tweetMessage[45];
	  	//  tweet that there are issues with SD at startup time
		sprintf(tweetMessage,"@v8surf SD Card failed on startup %lX", millis());
		tweet(tweetMessage);
	  Serial.println(tweetMessage);
	  return;
  }
  else
  {
	Serial.println("card initialized.");
  }


  delay(2000);

  //setupEthernet();	//DHCP IP address

  Serial.println(F("starting up"));

//  wdt_reset();
 // wdt_enable(WDTO_8S);

}

void loop()
{
	//wdt_reset();

	webPage();
	webPage();
	processData();
	//readSensors();

 // client.stop();


}

void processData()
{
	if((lastUpdate + 2000) < millis())
	{
		printFreeMem();
		//sendToSparkFun();

		lastUpdate = millis();

		if (fieldData[5] != String("Auto") && fieldData[5] != lastIP)
		{
			lastIP = fieldData[5];
			File dataFile = SD.open("ipLOG.txt", FILE_WRITE);
			// if the file is available, write to it:
			if (dataFile) {

				Serial.println("logging IP to SD");
				dataFile.println(fieldData[5]);
				dataFile.close();
			}

			dataFile = SD.open("ipLOGmil.txt", FILE_WRITE);
			if (dataFile)
			{
				dataFile.print(fieldData[5]);
				dataFile.print(" - HOT water is: ");
				dataFile.print(digitalRead(hotWaterPin));
				dataFile.print(" - @ Millis: ");
				dataFile.print(millis());
				dataFile.print(", millis since last log: ");
				dataFile.println((millis() - lastIpLog));
				dataFile.close();
				// print to the serial port too:
				Serial.println(fieldData[5]);
				lastIpLog = millis();
			}
			// if the file isn't open, pop up an error:
			else {

				if (lastTweetAboutSdCard + sdErrorTweetInterval < millis())
				{
				unsigned long ipLogMillis = millis();
				char tweetMessage[45];

				Serial.println("error opening IPlog.txt");
				//  tweet that there are issues??
				sprintf(tweetMessage,"@v8surf SD Card Failed or not there %d.%d.%d.%d %lX", rip[0], rip[1], rip[2], rip[3], ipLogMillis);
				tweet(tweetMessage);

				lastTweetAboutSdCard = millis();	// So I dont send loads of SD card faults
				}
			}
		}
	}

	if((lastAutoUpdate + autoUpdateInterval) < millis())
	{
		sendTweet();
		Serial.println(F("60 sec update"));
		printFreeMem();
		readSensors();
		fieldData[5] = String("Auto");			// set IP address to auto for logging
		//sendToSparkFun();
		Serial.println(fieldData[5]);
		lastAutoUpdate = millis();
	}
}

void printFreeMem()
{
	 Serial.print(F("freeMemory()="));
	 Serial.println(freeMemory());
}
void sortRipData()
{

/*
	Re: Get client's IP address from ethernet server?
	« Reply #19 on: May 21, 2010, 11:52:30 pm »
	Bigger Bigger   Smaller Smaller   Reset Reset
	I'll post it until Brett is able to release an "official" version:	http://www.tnhsmith.net/Misc/Ethernet-bh-mods-20100510.zip

	Here's a note from Brett, there also some code on the Rogue Robotics site that uses the getRemoteIP call.  Thanks once again Brett!

	Here's the revised "Ethernet" library.  It's definitely not official, but I will be working on it over time.  I'll get it into a code repository soon enough and integrate all of the improvements.
	To get the remote IP address, you need to call getRemoteIP() when you have the Client class from your server.available() call.
	e.g.
	Server s;
	Client c;
	byte rip[4];

	c = s.available();

	if (c)
	{
		c.getRemoteIP(rip);
		// rip[] now has 4 quads of the remote IP address }
*/
			fieldData[5] = String("Auto");			//set data to nothing so it is filled correctly only once below

			if (rip[0] == 0 && rip[1] == 0 && rip[2] == 0 && rip[3] == 0)
			{

			}
			else
			{
				fieldData[5] = String("");

				for (int bcount= 0; bcount < 4; bcount++)
				{
					//Serial.print(rip[bcount], DEC);
					//if (bcount<3) Serial.print(".");
					fieldData[5] += String(rip[bcount]);
					if (bcount<3) fieldData[5] += String(".");

					//rip[bcount] = 0;
				}
			}

			Serial.println(fieldData[5]);

}
void webPage()
{


	int isHotWaterOnOrOff = digitalRead(hotWaterPin);	// check state of pin (for instant tweets on change)


	EthernetClient client = server.available();
	//if (client)
	if(!client)
	{
		if (millis() - last404 > 999)		//
		{
			//			Serial.println(F("send 404"));
			//			printFreeMem();
			last404 = millis();
			//Serial.println(millis());
		}

		client.println(F("HTTP/1.1 404 OK"));
		client.println(F("Content-Type: text/html"));
		client.println(F("Connnection: close"));
		client.println();
		client.println(F("<!DOCTYPE HTML>"));
		client.println(F("<html><body>404</body></html>"));
	}
	else
	{
		// serve client

		Serial.println(F("into webpage.....++++++++++++++++++++++++++++++++++++++++++++"));
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		if (client) {

			int type = 0;
			while (client.connected()) {
				if (client.available()) {
					// GET, POST, or HEAD
					memset(buffer,0, sizeof(buffer)); // clear the buffer
					if(client.readBytesUntil('/', buffer,sizeof(buffer))){
						if(strcmp(buffer,"POST ") == 0){
							client.find("\n\r"); // skip to the body
							// find string starting with "pin", stop on first blank line
							// the POST parameters expected in the form pinDx=Y
							// where x is the pin number and Y is 0 for LOW and 1 for HIGH
							while(client.findUntil("pinD", "\n\r")){
								int pin = client.parseInt();       // the pin number
								int val = client.parseInt();       // 0 or 1
								pinMode(pin, OUTPUT);
								if(pin == 9){
									hotWaterPinState = val;

									if (val == 0)
									{
										fieldData[0] = String("OFF");
										digitalWrite(hotWaterPin, LOW);
										//ripTweet[0] = rip[0];	//save ip of PC that changed state to seperate array for tweeting
										//ripTweet[1] = rip[1];
										//ripTweet[2] = rip[2];
										//ripTweet[3] = rip[3];
									}
									else
									{
										fieldData[0] = String("ON");
										digitalWrite(hotWaterPin, HIGH);
										//ripTweet[0] = rip[0];	//save ip of PC that changed state to seperate array for tweeting
										//ripTweet[1] = rip[1];
										//ripTweet[2] = rip[2];
										//ripTweet[3] = rip[3];
									}

								}

							}
						}


						// http://forum.arduino.cc/index.php?PHPSESSID=kmlp7edmpu9jitvrl8avv1io71&topic=37565.15
						//client.remoteIP(rip);			// get remote IP address
						//getSn_DIPR(_sock, remoteIP);
						//return remoteIP;

						client.getRemoteIP(rip); // where rip is defined as byte rip[] = {0,0,0,0 };

						sendHeader(client,"Whanga");

						client.println(F("<h2><font color=#000000><b>Whanga weather data & Hot water control</b></h2>"));

						//client.print("<LABEL for='PIN'>PIN: </LABEL><INPUT type='password' id='password'><BR><INPUT type='submit' value='Send'>");

						if (hotWaterPinState == 1)
						{
							//create HTML button to control pin 9
							client.print(F("<form action='/' method='POST'><p><input type='hidden' name='pinD9'"));
							client.print(F(" value='0'><input type='submit' value='Turn Hot Water OFF'/></form>"));
							client.println(F("Press button to turn hot water OFF"));

						}

						else
						{
							//create HTML button to turn on pin 9
							client.print(F("<form action='/' method='POST'><p><input type='hidden' name='pinD9'"));
							client.print(F(" value='1'><input type='submit' value='Turn Hot water  ON'/></form>"));
							client.println(F("Press button to turn hot water ON"));

						}

						client.print(F("<br><br><br>"));		// create some white space

						client.print(F("Temperature: "));
						client.print(currentTemp);
						client.print(F(" deg"));
						client.print(F("<br>"));

						client.print(F("Humidity: "));
						client.print(currentHumidity);
						client.print(F("%"));
						client.print(F("<br>"));

						client.print(F("Current wind speed: "));
						client.print(currentWindSpeed);
						client.print(F(" KM/hr"));
						client.print(F("<br>"));

						client.print(F("Current wind direction: "));
						client.print(currentWindDirection);
						client.print(F(" deg"));
						client.print(F("<br>"));

						Serial.println(F("webpage data sent"));

						client.print(F("<br>"));
						client.print(F("<br>"));
						//client.print(F("<a href='https://data.sparkfun.com/streams/MGGaNdLE49u1lp3Kd3V1'>Logging Data</a> "));		//didnt really work that great

						client.print(F("<br>"));
						client.print(pageViews);
						client.print(F("<br>"));
						client.print(F("<a> millis = "));
						client.print(millis(), HEX);
						client.print(F("<br>"));
						client.print(F("<a> freemem = "));
						client.print(freeMemory());
						client.print(F("<br>"));
						client.print(F("<a> last IP "));
						client.print(fieldData[5]);
						client.print(F("<br>"));
						File dataFile = SD.open("ipLOGmil.txt");		// check if SD card is still OK
						if (dataFile)
						{
							client.print(F("<a> SD OK "));				// close file otherwise it chews up RAM
							dataFile.close();
						}
						else
						{
							client.print(F("<a> SD NOK "));
						}




						client.print(F("<br><br><br>"));

						client.println(F("</body></html>"));
						client.stop();

					}
				}
			}
			// give the web browser time to receive the data
			delay(1);
			client.stop();
			}


			//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			sortRipData();
			Serial.println(pageViews);
			//webPageLoops++;
			pageViews++;		// increment page counter
			sendTweet();
			Serial.println(F("out of webpage... +++++++++++++++++++++++++++++++++++++++++++"));
		}



}

void sendHeader(EthernetClient client, char *title)
{
	//wdt_reset();

  // send a standard http response header
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println(F("Refresh: 20"));  // refresh the page automatically every 20 seconds     //http://tronixstuff.com/2013/12/06/arduino-tutorials-chapter-16-ethernet/
  client.println();
  client.println(F("<!DOCTYPE HTML>"));
  client.println(F("<html>"));
  client.print(F("<html><head><title>"));
  client.print(title);
  client.println(F("</title><body>"));
 }

void sendToSparkFun()
{

	//wdt_reset();
	Serial.print(F("Sending to sparkfun...."));


/*
Format:
http://data.sparkfun.com/input/[publicKey]?private_key=[privateKey]&hotWaterState=[value]&humidity=[value]&temperature=[value]&windDirection=[value]&windSpeed=[value]
Example:
http://data.sparkfun.com/input/rood3GAnOXi034pg6XNr?private_key=jkkNnd2061U4K0gexXDz&hotWaterState=3.27&humidity=27.01&temperature=10.63&windDirection=19.26&windSpeed=5.46
If you would like to learn more about how to use data.sparkfun.com, please visit the documentation for more info.
*/

if (hotWaterPinState == 0)
{
	fieldData[0] = String("OFF");
}
else
{
	fieldData[0] = String("ON");
}

/*

// Gather data for sending to sparkfun:
//fieldData[0] = String(val);			// updated above in HTML code section
fieldData[1] = String(currentHumidity);
fieldData[2] = String(currentTemp);
fieldData[3] = String(currentWindDirection);
fieldData[4] = String(currentWindSpeed);

	// Make a TCP connection to remote host
	if (client.connect(serverToSendTo, 80))
	{
		// Post the data! Request should look a little something like:
		// GET /input/publicKey?private_key=privateKey&light=1024&switch=0&name=Jim HTTP/1.1\n
		// Host: data.sparkfun.com\n
		// Connection: close\n
		// \n
		client.print(F("GET /input/"));
	Serial.print(F("GET /input/"));
		client.print(publicKey);
	Serial.print(publicKey);
		client.print(F("?private_key="));
	Serial.print(F("?private_key="));
		client.print(privateKey);
	Serial.print(privateKey);
		for (int i=0; i<NUM_FIELDS; i++)
		{
			client.print(F("&"));
		Serial.print(F("&"));
			client.print(fieldNames[i]);
		Serial.print(fieldNames[i]);
			client.print(F("="));
		Serial.print(F("="));
			client.print(fieldData[i]);
		Serial.print(fieldData[i]);
		}
		client.println(F(" HTTP/1.1"));
		client.print(F("Host: "));
		client.println(serverToSendTo);
		client.println(F("Connection: close"));
		//client.println();
	}
	else
	{
		Serial.println(F("Connection failed"));
	}

	// Check for a response from the server, and route it
	// out the serial port.


	while (client.connected())
	{
		if ( client.available() )
		{
			char c = client.read();
			Serial.print(c);
		}
	}
	*/
	Serial.println(F("send to SF DONE"));
	client.stop();
}

void setupEthernet()
{

	//wdt_reset();

	Serial.println(F("Setting up Ethernet..."));

	// start the Ethernet connection:
	if (Ethernet.begin(mac) == 0) {
		Serial.println(F("Failed to configure Ethernet using DHCP"));
		// no point in carrying on, so do nothing forevermore:
		// try to congfigure using IP address instead of DHCP:
		Ethernet.begin(mac, ip);
	}
	Serial.print(F("My IP address: "));
	Serial.println(Ethernet.localIP());
	// give the Ethernet shield a second to initialize:
	delay(1000);
}

void readSensors()
{
	//wdt_reset();
	/*

	//shift data in array to make room for new data
	for(int i = chartArrayLength; i >= 0; i--)
	{
		humidityArray[i]		= humidityArray[i-1];

	}

	for(int i = chartArrayLength; i >= 0; i--)
	{
		windSpeedArray[i]		= windSpeedArray[i-1];

	}
	for(int i = chartArrayLength; i >= 0; i--)
	{
		windDirectionArray[i]	= windDirectionArray[i-1];
	}
	for(int i = chartArrayLength; i >= 0; i--)
	{
		temperatureArray[i]		= temperatureArray[i-1];
	}
	*/
	currentTemp					= random(46);
	currentHumidity				= random(101);
	currentWindSpeed			= random(181);
	currentWindDirection		= random(360);

	temperatureArray[0]			= currentTemp;
	humidityArray[0]			= currentHumidity;
	windSpeedArray[0]			= currentWindSpeed;
	windDirectionArray[0]		= currentWindDirection;

	sendCurrentOutSerial();


	//getAverages();

}

void sendCurrentOutSerial()
{


Serial.println(F("********"));
Serial.print(F("Temp: "));
Serial.println(currentTemp);
Serial.print(F("Humidity: "));
Serial.println(currentHumidity);
Serial.print(F("Windspeed: "));
Serial.println(currentWindSpeed);
Serial.print(F("Wind Direction: "));
Serial.println(currentWindDirection);
Serial.println(F("********"));
Serial.println(F(""));

}
void drawGoogleChart()
{
	 //wdt_reset();

		client.println(F("<head>"));
		client.println(F("<script type='text/javascript' src='https://www.google.com/jsapi'></script>"));
		client.println(F("<script type='text/javascript'>"));
		client.println(F("google.load('visualization', '1', {packages:['corechart']});"));
		client.println(F("google.setOnLoadCallback(drawChart);"));
		client.println(F("function drawChart() {"));

//*************DATA***************************************************************************************************
/*
		client.println("var data = google.visualization.arrayToDataTable([");
		client.println("['Hours', 'Temperature', 'Humidity', 'Wind Speed', 'Wind Direction'],");

		for (int chartArrayPosition = chartArrayLength; chartArrayPosition >= 0; chartArrayPosition--)
		{
			//client.println("*['*48*',* 10*,* 45*,* 12*,* 80*],*");
			client.print("['");
			client.print(chartArrayPosition);
			client.print("', ");
			client.print(temperatureArray[chartArrayPosition]);
			client.print(", ");
			client.print(humidityArray[chartArrayPosition]);
			client.print(", ");
			client.print(windSpeedArray[chartArrayPosition]);
			client.print(", ");
			client.print(windDirectionArray[chartArrayPosition]);
			client.println("],");
		}
		client.println("]);");

*/


	// Temperature ////////////////////////////////////////////////////////////////
	client.println(F("var dataTemperature = google.visualization.arrayToDataTable(["));
	client.println(F("['Hours2', 'Temperature2'],"));

		for (int chartArrayPosition = chartArrayLength; chartArrayPosition >= 0; chartArrayPosition--)
		{

			//client.println("*['*48*',* 10*,* 45*,* 12*,* 80*],*");
			client.print("['");
			client.print(chartArrayPosition);
			client.print("', ");
			client.print(temperatureArray[chartArrayPosition]);
			client.println("],");
		}
		client.println("]);");



/*

		// Humidity ////////////////////////////////////////////////////////////////
		client.println("var dataHumidity = google.visualization.arrayToDataTable([");
		client.println("['Hours3', 'Humidity3'],");

		for (int chartArrayPosition = chartArrayLength; chartArrayPosition >= 0; chartArrayPosition--)
	{

			//client.println("*['*48*',* 10*,* 45*,* 12*,* 80*],*");
			client.print("['");
			client.print(chartArrayPosition);
			client.print("', ");
			client.print(humidityArray[chartArrayPosition]);
			client.println("],");
	}
	client.println("]);");

	*/

//*************OPTIONS***************************************************************************************************
	/*
// ALL Data		///////////////////////////////////////////////////////////////////////////////////////////
client.println("var options = {");
	client.println("title: 'Weather data last 50 readings',");
	//client.println("curveType: 'function',");
client.println("};");
*/

//Temperature	///////////////////////////////////////////////////////////////////////////////////////////
client.println(F("var optionsTemperature = {"));
	client.println(F("title: 'Temperature data last 50 readings',"));
	//client.println("curveType: 'function',");
client.println(F("};"));



/*
		//Humidity	///////////////////////////////////////////////////////////////////////////////////////////
		client.println("var optionsHumidity = {");
			client.println("title: 'Humidity data last 50 readings',");
			//client.println("curveType: 'function',");
		client.println("};");

		*/
//*************DRAW CHART***************************************************************************************************
/*
		//ALL Data	///////////////////////////////////////////////////////////////////////////////////////////
		client.println("var chart = new google.visualization.LineChart(document.getElementById('chart_div'));");
		client.println("chart.draw(data, options);");
		client.println("}");
*/

		//Temperature	///////////////////////////////////////////////////////////////////////////////////////////
		client.println(F("var chartTemperature = new google.visualization.LineChart(document.getElementById('chart_div2'));"));
		client.println(F("chartTemperature.draw(dataTemperature, optionsTemperature);"));
		client.println(F("}"));

/*
		//Humidity	///////////////////////////////////////////////////////////////////////////////////////////
		client.println("var chartHumidity = new google.visualization.LineChart(document.getElementById('chart_div3'));");
		client.println("chartHumidity.draw(dataHumidity, optionsHumidity);");
		client.println("}");
*/

	client.println(F("</script>"));
	client.println(F("</head>"));
	client.println(F("<body>"));
	client.println(F("<div id='chart_div' style='width: 1000px; height: 300px;'></div>"));
	client.println(F("<div id='chart_div2' style='width: 1000px; height: 300px;'></div>"));
	client.println(F("<div id='chart_div3' style='width: 1000px; height: 300px;'></div>"));
	client.println(F("</body>"));

}

void getAverages()
{


	Serial.print(F("in Avgs...."));


	//wdt_reset();

	unsigned long windSpeedRunningAvg = 0 ;
	unsigned long windDirectionRunningAvg = 0;
	unsigned long temperatureRunningAvg = 0;
	unsigned long humidityRunningAvg = 0;

	for(int i = 0; i < chartArrayLength; i++)
	{
		windSpeedRunningAvg		+= windSpeedArray[i];
		//Serial.println(windSpeedArray[i]);
		windDirectionRunningAvg	+= windDirectionArray[i];
		temperatureRunningAvg	+= temperatureArray[i];
		//Serial.println(temperatureArray[i]);
		humidityRunningAvg		+= humidityArray[i];
	}

	/*
	Serial.println(temperatureRunningAvg);
	Serial.println(humidityRunningAvg);
	Serial.println(windSpeedRunningAvg);
	Serial.println(windDirectionRunningAvg);
		*/

	currentWindSpeedAverage		= windSpeedRunningAvg / chartArrayLength;
	currentWindDirectionAverage	= windDirectionRunningAvg / chartArrayLength;
	currentTempAverage			= temperatureRunningAvg / chartArrayLength;
	currentHumidityAverage		= humidityRunningAvg / chartArrayLength;

	Serial.println(F("out Avgs"));
}

void tweet(char msg[])
{
	Serial.println(F("connecting ..."));
	if (twitter.post(msg))
	{
		// Specify &Serial to output received response to Serial.
		// If no output is required, you can just omit the argument, e.g.
		// int status = twitter.wait();
		int status = twitter.wait(&Serial);
		if (status == 200)
		{
			Serial.println(F("OK."));
		}
		else
		{
			Serial.print(F("failed : code "));
			Serial.println(status);
		}
	}
	else
	{
		Serial.println(F("connection failed."));
	}
}

void sendTweet()
{

	unsigned int randomDelay = random(123);
	delay(randomDelay);		// so twitter hopefully doesnt get a duplicate tweet on startup.  They dont allow that for some reason?

	if (lastHotWaterState != digitalRead(hotWaterPin))
	{

	Serial.println(F("last pinstate different "));

		/*
		// create message to tweet ================================================================
		char tweetMessage[45];
		// assemble message to send.
		//sprintf(tweetMessage, "Pin analogue zero reads: %d. @username.", analogZero); // change @username to your twitter account name
		sprintf(tweetMessage, "Change from IP %d.%d.%d.%d to %d @v8surf",ip0, ip1, ip2, ip3, pinStateString); // change @username to your twitter account name
		// ============================15============15==4===3====9==========8=============================
		*/
		char tweetMessage[45];
		unsigned long tweetMillis = millis();
		if (digitalRead(hotWaterPin) == 1)
		{
			sprintf(tweetMessage,"@v8surf %d.%d.%d.%d HW: ON %lX", rip[0], rip[1], rip[2], rip[3], tweetMillis);
		}
		else
		{
			sprintf(tweetMessage,"@v8surf %d.%d.%d.%d HW: OFF %lX", rip[0], rip[1], rip[2], rip[3], tweetMillis);
		}





		Serial.println(F("tweeting... "));
		Serial.println(tweetMessage);				// send tweet out serial

		tweet(tweetMessage);					// send tweet to twitter

		lastHotWaterState = digitalRead(hotWaterPin);

		Serial.println(F("tweeting... completed"));
	}

}