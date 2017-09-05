#include <SoftwareSerial.h>
#include <EEPROM.h>

#define GSM_RX 3  // пин RX на модуле подключаем к указаному пину на Ардуино TX
#define GSM_TX 2  // пин TX на модуле подключаем к указаному пину на Ардуино RX

#define SECRET 45

SoftwareSerial gsm(GSM_TX, GSM_RX); // установка контактовGSM_TX->RX и GSM_RX->TX для программного порта

#define RELE_ONE 6 // Пин для подключения реле 1
#define RELE_TWO 5  // Пин для подключения реле 2


bool oneStart = true;                                             //  статус первого запуска устройства
String phone = "";                                                //  тут будет номер мастер тел в виде строки
int phonePref, phoneCode, phoneOne, phoneTwo;
String ch1, ch2, ch3, ch4;

int EEPROM_int_read(int addr)
{
    byte raw[2];
    for (byte i = 0; i < 2; i++) raw[i] = EEPROM.read(addr + i);
    int &num = (int&)raw;
    return num;
}

void EEPROM_int_write(int addr, int num)
{
    byte raw[2];
    (int&)raw = num;
    for (byte i = 0; i < 2; i++) EEPROM.write(addr + i, raw[i]);
}

void sms(String text, String phone)
{
    gsm.println("AT+CMGF=1"); delay(500);
    gsm.println("AT+CMGS=\"+" + phone + "\""); delay(500);
    gsm.println(text); delay(500);
    gsm.print((char)26); delay(2000);  clearsms();
}

void clearsms()
{
  gsm.println("AT+CMGD=1");delay(500);
  gsm.println("AT+CMGDA");delay(500);
}

void setup()
{
    pinMode(RELE_ONE, OUTPUT); digitalWrite(RELE_ONE, LOW);
    pinMode(RELE_TWO, OUTPUT); digitalWrite(RELE_TWO, LOW);

    Serial.begin(9600); // Инициализация UART

    gsm.begin(9600); // Инициализация GSM

    delay(10000); // Ожидание регистрации в сети
    
    // настройка приема сообщений
    gsm.println("AT+CMGF=1"); delay(500); // устанавливаем текстовый режимсмс-сообщения
    gsm.println("AT+IFC=1, 1"); delay(500); // устанавливаем программныйконтроль потоком передачи данных
    gsm.println("AT+CPBS=\"SM\""); delay(500); // открываем доступ кданным телефонной книги SIM-карты
    gsm.println("AT+CNMI=1,2,2,1,0"); delay(500); // включает оповещение оновых сообщениях, новые сообщения приходят в следующем формате: +CMT:"<номер телефона>", "", "<дата, время>", ( +CMT:"+380xxxxxxxxx","","17/02/04,04:44:09+08" ) сразу за ним идет самосообщение с новой строки
    gsm.println("AT+CSCS=\"GSM\""); delay(500); // устанавливаем кодировку

    // Удаляем все сообщения
    gsm.println("AT+CMGD=1"); delay(2500); 

    //  Если не первый запуск
    if (EEPROM.read(0) == SECRET)
    {
        //  Вытаскиваем номер из епром и кидаем в переменную номер тел...  и преобразовываем телефон в строку
        phonePref = EEPROM_int_read(1);
        phoneCode = EEPROM_int_read(3);
        phoneOne = EEPROM_int_read(5);
        phoneTwo = EEPROM_int_read(7);
    
        if (String(phonePref).length() == 1) ch1 = String(phonePref);
    
        if (String(phoneCode).length() == 1)
        {
            ch2 = "00" + String(phoneCode);
        } 
        else if (String(phoneCode).length() == 2)
        {
            ch2 = "0" + String(phoneCode);
        } 
        else 
        {
            ch2 = String(phoneCode);
        }
  
        if (String(phoneOne).length() == 1)
        {
            ch3 = "00" + String(phoneOne);
        } else if (String(phoneOne).length() == 2)
        {
            ch3 = "0" + String(phoneOne);
        } 
        else 
        {
            ch3 = String(phoneOne);
        }
    
        if (String(phoneTwo).length() == 1)
        {
            ch4 = "000" + String(phoneTwo);
        }
        else if (String(phoneTwo).length() == 2)
        {
            ch4 = "00" + String(phoneTwo);
        }
        else if (String(phoneTwo).length() == 3)
        {
            ch4 = "0" + String(phoneTwo);
        } 
        else
        {
            ch4 = String(phoneTwo);
        }
    
        phone = ch1 + ch2 + ch3 + ch4;
    
        //  Отправляем СМС что всё в норме на номер мастер тел
        sms("OK", phone);  // Отправляем СМС на номер +79517956505
        Serial.print("Master phone: +");
        Serial.println(phone);
        
        //  Меняем статус программы на стандартный режим работы
        oneStart = false;
    }
    else
    {
        Serial.println("Not master phone");
    }
    
    delay(5000);
    gsm.flush();

    Serial.println("Settings complete!!!");
    Serial.println("----------------------------------------------------------");
}

void loop() 
{
    // Цицл будет работать только пока первый запуск и отсутствует номер мастер телефона
    while (oneStart)
    {
        // Если GSM модуль что-то послал
        if (gsm.available())
        {
            delay(200);  // Ожидаем заполнения буфера
            
            uint8_t ch = 0;
            String val = "";
            
            // Cохраняем входную строку в переменную val
            while (gsm.available())
            {
                ch = gsm.read();
                val += char(ch);
                delay(10);
            }
      
            // Eсли звонок обнаружен, то проверяем номер
            if (val.indexOf("RING") > -1)
            {
                gsm.println("ATH"); // Разрываем связь
               
                //  Переводим в число номер телефона в виде 4-х частей
                phonePref = (val.substring(18, 19)).toInt();
                phoneCode = (val.substring(19, 22)).toInt();
                phoneOne =  (val.substring(22, 25)).toInt();
                phoneTwo =  (val.substring(25, 29)).toInt();
        
                //  Записываем в ЕЕПРОМ
                EEPROM.write(0, SECRET);
        
                EEPROM_int_write(1, phonePref);
                EEPROM_int_write(3, phoneCode);
                EEPROM_int_write(5, phoneOne);
                EEPROM_int_write(7, phoneTwo);
        
                if (String(phonePref).length() == 1)
                {
                    ch1 = String(phonePref);
                }
                                
                if (String(phoneCode).length() == 1) 
                {
                    ch2 = "00" + String(phoneCode);
                }
                else if (String(phoneCode).length() == 2)
                {
                    ch2 = "0" + String(phoneCode);
                }
                else 
                {
                    ch2 = String(phoneCode);
                }
                
                if (String(phoneOne).length() == 1)
                {
                    ch3 = "00" + String(phoneOne);
                }
                else if (String(phoneOne).length() == 2)
                {
                    ch3 = "0" + String(phoneOne);
                }
                else 
                {
                    ch3 = String(phoneOne);
                }
                
                if (String(phoneTwo).length() == 1)
                {
                    ch4 = "000" + String(phoneTwo);
                } 
                else if (String(phoneTwo).length() == 2)
                {
                    ch4 = "00" + String(phoneTwo);
                }
                else if (String(phoneTwo).length() == 3) 
                {
                    ch4 = "0" + String(phoneTwo);
                } 
                else
                {
                    ch4 = String(phoneTwo);
                }
                
                phone = ch1 + ch2 + ch3 + ch4;
        
                Serial.print("SAVE MASTER +");
                Serial.println(phone);
        
                //  Отправляем информацию о том что номер сохранен
                sms("Number " + phone + " save", phone);  // Отправляем СМС на номер
        
                //  номер записан - можно сменить статус программы на стандартный режим работы
                oneStart = false;

                delay(10000);
                break;
            }
      
            val = "";  // Очищаем переменную команды
            gsm.flush();
        }
    }

    delay(10000);
    gsm.flush();
    
    // Цицл стандартный режим работы
    while (!oneStart)
    {
        if (gsm.available())
        {
            delay(200);  // Ожидаем заполнения буфера
            
            uint8_t ch = 0;
            String val = "";

            while (gsm.available())
            {
                ch = gsm.read();
                val += char(ch);
                delay(10);
            }
           
            //----------------------- ЕСЛИ ВЫЗОВ -----------------------//
            if (val.indexOf("RING") > -1)
            {
                gsm.println("ATH"); // Разрываем связь
                
                //  ЕСЛИ ВЫЗОВ ОТ ХОЗЯИНА
                if (val.indexOf(phone) > -1)
                {
                    delay(10000);
                    
                    sms("CALL",phone);
                    Serial.println("CALL");
                    
                }
            }
    
            //----------------------- ЕСЛИ СМС -----------------------//
            if (val.indexOf("+CMT") > -1)
            {
                //  ЕСЛИ SMS ОТ ХОЗЯИНА
                if (val.indexOf(phone) > -1)
                {
                    if (val.indexOf("SMS") > -1)
                    {
                        Serial.println("SMS");
                        
                        sms("SMS",phone);
                    }
                    
                    if (val.indexOf("ON1") > -1)
                    {
                        Serial.println("ON1");
                        digitalWrite(RELE_ONE, HIGH);
                        
                        sms("ON1",phone);
                    }
    
                    if (val.indexOf("OFF1") > -1)
                    {
                        Serial.println("OFF1");
                        digitalWrite(RELE_ONE, LOW);
                        
                        sms("OFF1", phone);
                    }
                    
                    if (val.indexOf("ON2") > -1)
                    {
                        Serial.println("ON2");
                        digitalWrite(RELE_TWO, HIGH);
                        
                        sms("ON2", phone);
                    }
    
                    if (val.indexOf("OFF2") > -1)
                    {
                        Serial.println("OFF2");
                        digitalWrite(RELE_TWO, LOW);
                        
                        sms("OFF2", phone);
                    }
                    
                    if (val.indexOf("ALLON") > -1)
                    {
                        Serial.println("ALLON");
                        digitalWrite(RELE_ONE, HIGH);
                        digitalWrite(RELE_TWO, HIGH);
                        
                        sms("ALLON", phone);
                    }
    
                    if (val.indexOf("ALLOFF") > -1)
                    {
                        Serial.println("ALLOFF");
                        digitalWrite(RELE_ONE, LOW);
                        digitalWrite(RELE_TWO, LOW);
                        
                        sms("ALLOFF", phone);
                    }
    
                    if (val.indexOf("rp3") > -1)
                    {
                        Serial.println("rp3");
          
                        digitalWrite(RELE_ONE, HIGH);
                        digitalWrite(RELE_TWO, HIGH);
                        delay(10000);
                        digitalWrite(RELE_ONE, LOW);
                        digitalWrite(RELE_TWO, LOW);
          
                        sms("OFF3", phone);
                    }    
                }
    
                if (val.indexOf("PASSWORD") > -1)
                {
                    Serial.println("PASSWORD");
                    sms("PASSWORD", phone);
                    EEPROM.write(0, SECRET + 1);
                    return;
                }
            }
    
            val = "";  // Очищаем переменную команды
            gsm.flush();
        }
    }
}
