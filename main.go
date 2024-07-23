//mosquitto_sub -p 8883 --cafile ca.crt --cert client.crt --key client.key -h 192.168.1.14 -t "weather"
//mosquitto_pub -p 8883 --cafile ca.crt --cert client.crt --key client.key -h 192.168.1.14 -m "hello" -t "weather"

package main

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"time"

	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/gin-gonic/gin"
)

type MyMessage struct {
	Photo       float32 `json:"photo"`
	Rain        int     `json:"rain"`
	Humidity    float32 `json:"humidity"`
	Temperature float32 `json:"temperature"`
	HeatIndex   float32 `json:"heatIndex"`
	WindSpeed   float32 `json:"windSpeed"`
}

func main() {
	message := make(chan string)
	send := make(chan string)
	go ConnectToBroker(message)
	r := gin.Default()

	r.GET("/events", func(c *gin.Context) { eventsHandler(c, send, message) })
	// http.HandleFunc("/events", eventsHandler)
	fmt.Println("Listening on :8080")
	if err := r.RunTLS(":8080", "servermain.crt", "servermain.key"); err != nil {
		log.Fatalf("Failed to start HTTP server: %v", err)
	}
}

func eventsHandler(c *gin.Context, send chan string, message chan string) {
	c.Header("Content-Type", "text/event-stream")
	c.Header("Cache-Control", "no-cache")
	c.Header("Connection", "keep-alive")
	c.Header("Access-Control-Allow-Origin", "*")
	c.Header("Access-Control-Expose-Headers", "Content-Type")
	// c.Header("X-Accel-Buffering", "no")
	for {
		select {
		case <-c.Request.Context().Done():
			return
		case t := <-message:
			fmt.Println(t)
			// elapsed := time.Since(start)
			// Convert elapsed time to seconds
			// miliSeconds := elapsed.Milliseconds()
			// heartBeat := 60 / (miliSeconds + 10)
			// CalHeart := CaloriesConsamptin(float32(heartBeat))
			// // CalSpeed := CalorinesConsumptionMET(float32(t.Speed))
			// start = time.Now()
			// // totalCals := CalHeart + CalSpeed
			// stringValue := fmt.Sprintf("%f", totalCals)
			// dataToSend := fmt.Sprintf("Speed: %.2f Calorines Consumption: %s", t.Speed, stringValue)
			// dataToSend := HealthData{HeartBit: 35.2, Speed: 25.3}
			// fmt.Fprintf(c.Writer, "data: %s\n\n", dataToSend)
			// time.Sleep(20 * time.Millisecond)
			// c.Writer.Flush()

		default:
			time.Sleep(100 * time.Millisecond)
		}
	}
}

func CaloriesConsamptin(heartBit float32) float32 {

	return (1 * (heartBit*0.6309 - 40*0.1988 + 80*0.2017 - 55.0969) * (4.184 / 1000))
}

func ConnectToBroker(message chan string) {

	// MQTT broker detailsgo run m
	broker := "tls://192.168.1.16:8883"
	topic := "weather"
	clientID := "mqtt-subscriber"

	// Load client cert
	cert, err := tls.LoadX509KeyPair("./mqtt/config/cert/clientsse.crt", "./mqtt/config/cert/clientsse.key")
	if err != nil {
		log.Fatalf("Failed to load client certificate: %v", err)
	}

	// Load CA cert
	caCert, err := os.ReadFile("./mqtt/config/cert/ca.crt")
	if err != nil {
		log.Fatalf("Failed to read CA certificate: %v", err)
	}
	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(caCert)

	// Create TLS config
	tlsConfig := &tls.Config{
		Certificates:       []tls.Certificate{cert},
		RootCAs:            caCertPool,
		InsecureSkipVerify: true, //<<<<<<<<<<<<<<<<<< SOS for self sighn
	}

	opts := MQTT.NewClientOptions()
	opts.AddBroker(broker)
	opts.SetClientID(clientID)
	opts.SetTLSConfig(tlsConfig)

	// Define the message handler
	opts.SetDefaultPublishHandler(func(client MQTT.Client, msg MQTT.Message) {

		fmt.Printf("Received message: %s from topic: %s\n", msg.Payload(), msg.Topic())

		var myMessage MyMessage

		err := json.Unmarshal(msg.Payload(), &myMessage)
		if err != nil {
			log.Printf("Error deserializing message: %v", err)
			return
		}

		fmt.Println(myMessage.Photo, myMessage.Rain)

		// strMsg := strings.Split(string(msg.Payload()), "-------")
		// floatSpeed, err := strconv.ParseFloat(strMsg[1], 32)
		// if err != nil {
		// 	fmt.Println(err)
		// }
		// intHeart, err := strconv.ParseFloat(strMsg[1], 32)
		// if err != nil {
		// 	fmt.Println(err)
		// }
		// hd := HealthData{HeartBit: 11.5, Speed: 32.5}
		// fmt.Println(hd, "ASADASDASDASD")
		// message <- "hello"

	})

	// Create and start an MQTT client
	client := MQTT.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		log.Fatalf("Failed to connect to MQTT broker: %v", token.Error())
	}

	// Subscribe to the topic
	if token := client.Subscribe(topic, 1, nil); token.Wait() && token.Error() != nil {
		log.Fatalf("Failed to subscribe to topic: %v", token.Error())
	}

	// Keep the connection open
	for {
		time.Sleep(1 * time.Second)
	}
}
