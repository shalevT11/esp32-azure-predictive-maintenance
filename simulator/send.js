require('dotenv').config(); //load .env into process.env
const { Client, Message } = require('azure-iot-device') //client + padd the message
const { Mqtt } = require('azure-iot-device-mqtt'); //MQTT trafic

const connectionString = process.env.DEVICE_CONNECTION_STRING; //the secret from the .env


//build client from the string (id+key) with MQTT protocol
async function main() {
    const client = Client.fromConnectionString(connectionString, Mqtt); //identity + how to connect
    await client.open(); //open the connection (handshake + auth)
    console.log('Connected to IoT Hub, sending telemetry...');

    let seq = 0;
    setInterval(async () => {
        const spike = Math.random() < 0.05;
        const value = spike ? 1.5 : 1.0 + Math.random() *0.3;
        const payload = JSON.stringify({ seq: seq++, value}); //the msg as string in JSON

        const message = new Message(payload); //cast to Message
        message.contentType = 'application/json'; //hint to the client its JSON
        message.contentEncoding = 'utf-8';

        await client.sendEvent(message); //pub to the Hub (device -> cloud)
        console.log('sent:', payload);
    }, 10000);  //every 10 sec send to cloud

}

main().catch((err) => console.error('Error:', err)); //if some faild - into log