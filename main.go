// mosquitto_sub -p 8883 --cafile ca.crt --cert client.crt --key client.key -h 192.168.1.14 -t "weather"
// mosquitto_pub -p 8883 --cafile ca.crt --cert client.crt --key client.key -h 192.168.1.14 -m "hello" -t "weather"
// Final
package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"slices"
	"time"

	"github.com/BurntSushi/toml"
	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type Config struct {
	Database struct {
		Server   string
		User     string
		Password string
		Port     string
	}
	Broker struct {
		Ip       string
		Port     string
		Topic    string
		ClientId string
	}
}

type WeatherData struct {
	Photo       float32 `json:"photo"`
	Rain        int     `json:"rain"`
	Humidity    float32 `json:"humidity"`
	Temperature float32 `json:"temperature"`
	HeatIndex   float32 `json:"heatIndex"`
	WindSpeed   float32 `json:"windSpeed"`
	TimeData    int32   `json:"timeData"`
}

type ValsToReturn struct {
	Photo       []float32
	Rain        []int
	Humidity    []float32
	Temperature []float32
	HeatIndex   []float32
	WindSpeed   []float32
}

// var client *mongo.Client
var ctx = context.TODO()
var collection *mongo.Collection
var config Config

var uri string

func main() {
	if _, err := toml.DecodeFile("config.toml", &config); err != nil {
		log.Fatal(err)
	}

	//"mongodb://root:1234@localhost:27017"
	uri = fmt.Sprintf("mongodb://%s:%s@%s:%s", config.Database.User, config.Database.Password, config.Database.Server, config.Database.Port)
	fmt.Println(uri)
	// time.Sleep(100 * time.Second)
	message := make(chan string)
	go ConnectToBroker(message)

	r := gin.Default()
	r.Use(cors.New(cors.Config{
		AllowOrigins: []string{"*"}, // You can specify allowed origins here
		AllowMethods: []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowHeaders: []string{"Origin", "Content-Type", "Authorization"},
	}))
	r.GET("/getdata", Getdata)
	// http.HandleFunc("/events", eventsHandler)
	fmt.Println("Listening on :8080")
	if err := r.RunTLS(":8080", "servermain.crt", "servermain.key"); err != nil {
		log.Fatalf("Failed to start HTTP server: %v", err)
	}
}

func Getdata(c *gin.Context) {
	c.Header("Content-Type", "text/html; charset=utf-8")
	c.Header("Access-Control-Allow-Origin", "*")

	collection := ConnectToMongo()
	findOptions := options.Find()
	findOptions.SetSort(bson.D{{Key: "_id", Value: -1}})
	findOptions.SetLimit(100)

	cur, err := collection.Find(ctx, bson.D{{}}, findOptions)
	if err != nil {
		log.Fatal(err)
	}
	var results []WeatherData
	for cur.Next(ctx) {

		var result WeatherData
		err := cur.Decode(&result)
		if err != nil {
			log.Fatal(err)
		}
		results = append(results, result)
	}

	if err := cur.Err(); err != nil {
		log.Fatal(err)
	}
	slices.Reverse(results)
	// Close the cursor once finished
	dTs := ValsToReturn{}
	for _, j := range results {
		// Humidity    []float32
		// WindSpeed   float32
		dTs.HeatIndex = append(dTs.HeatIndex, j.HeatIndex)
		dTs.Temperature = append(dTs.Temperature, j.Temperature)
		dTs.Photo = append(dTs.Photo, j.Photo)
		if j.Rain == 0 {
			dTs.Rain = append(dTs.Rain, 1)
		} else if j.Rain == 1 {
			dTs.Rain = append(dTs.Rain, 0)
		}

		dTs.WindSpeed = append(dTs.WindSpeed, float32(j.WindSpeed))
		dTs.Humidity = append(dTs.Humidity, j.Humidity)
	}
	cur.Close(ctx)

	// Respond with the results
	c.JSON(http.StatusOK, dTs)

	// Disconnect from MongoDB

	// fmt.Println("Connection to MongoDB closed.")
}

func ConnectToMongo() *mongo.Collection {
	serverAPI := options.ServerAPI(options.ServerAPIVersion1)
	opts := options.Client().ApplyURI(uri).SetServerAPIOptions(serverAPI)
	client, err := mongo.Connect(ctx, opts)
	if err != nil {
		panic(err)
	}

	var result bson.M
	if err := client.Database("admin").RunCommand(ctx, bson.D{{Key: "ping", Value: 1}}).Decode(&result); err != nil {
		panic(err)
	}
	fmt.Println("Pinged your deployment. You successfully connected to MongoDB!")
	collection = client.Database("weather").Collection("data")
	return collection
}
func ConnectToBroker(message chan string) {
	collection = ConnectToMongo()
	// MQTT broker detailsgo run m
	//tls://192.168.1.16:8883
	broker := fmt.Sprintf("tls://%s:%s", config.Broker.Ip, config.Broker.Port)
	fmt.Println(broker, "AAAA")
	topic := config.Broker.Topic
	clientID := config.Broker.ClientId

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
		fmt.Println("!")
		var wd WeatherData

		err := json.Unmarshal(msg.Payload(), &wd)
		if err != nil {
			log.Printf("Error deserializing message: %v", err)
			return
		}
		now := time.Now()

		// Get the Unix timestamp (seconds since January 1, 1970 UTC)
		epoch := now.Unix()
		wd.TimeData = int32(epoch)
		insertResult, err := collection.InsertOne(ctx, wd)
		if err != nil {
			log.Fatal(err, "<<<<<<<<<<<<<<<<")
		}
		fmt.Println("Inserted a document: ", insertResult.InsertedID)

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
