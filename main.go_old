package main

import (
	"context"
	"fmt"
	"log"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

var client *mongo.Client
var errclient error

var weatherData struct {
	Speed      float32 `json:"speed"`
	Temp       float32 `json:"temp"`
	Humidity   float32 `json:"humidity"`
	Rain       bool    `json:"rain"`
	Visibility int     `json:"visibility"`
	Time       int64   `json:"time"`
}

func init() {
	username := "root"
	password := "1234"
	clusterAddress := "localhost:27017" // e.g., "localhost:27017" or "cluster0.mongodb.net"

	// Create the connection URI
	uri := fmt.Sprintf("mongodb://%s:%s@%s", username, password, clusterAddress)

	// Set client options
	clientOptions := options.Client().ApplyURI(uri)

	// Connect to MongoDB
	client, errclient = mongo.Connect(context.TODO(), clientOptions)
	if errclient != nil {
		log.Fatal(errclient)
	}

}

func main() {
	PingDB(client)
	router := gin.Default()
	router.POST("/insert", insertDocument)
	router.GET("/retrieve", retrieveDocument)
	router.Run(":8080")
}

func insertDocument(c *gin.Context) {
	wd := weatherData
	err := c.ShouldBindJSON(&wd)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	wd.Time = time.Now().Unix()
	collection := client.Database("weather").Collection("weatherdata")
	insertResult, err := collection.InsertOne(context.TODO(), wd)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"insertedID": insertResult.InsertedID})

}

func retrieveDocument(c *gin.Context) {
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	findOptions := options.Find()
	findOptions.SetSort(bson.D{{Key: "time", Value: -1}})
	findOptions.SetLimit(5)
	collection := client.Database("weather").Collection("weatherdata")

	cur, err := collection.Find(ctx, bson.D{}, findOptions)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	results := []bson.M{}
	for cur.Next(ctx) {
		var result bson.M
		err := cur.Decode(&result)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
			return
		}
		results = append(results, result)
		// fmt.Println(result)
	}
	fmt.Println(results, "<<<<<<<<<<<<<<<<")
	c.JSON(http.StatusOK, results)
}

func DisconectDB(client *mongo.Client) {

	err := client.Disconnect(context.TODO())
	if err != nil {
		log.Fatal(err)
	}

}

func PingDB(client *mongo.Client) {
	err := client.Ping(context.TODO(), nil)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("DB IS CONNECTED")

}

// package main

// import (
//     "context"
//     "fmt"
//     "log"
//     "time"

//     "go.mongodb.org/mongo-driver/bson"
//     "go.mongodb.org/mongo-driver/mongo"
//     "go.mongodb.org/mongo-driver/mongo/options"
// )

// func main() {
//     // Replace these with your MongoDB credentials
//     username := "yourUsername"
//     password := "yourPassword"
//     clusterAddress := "yourClusterAddress" // e.g., "localhost:27017" or "cluster0.mongodb.net"

//     // Create the connection URI
//     uri := fmt.Sprintf("mongodb://%s:%s@%s", username, password, clusterAddress)

//     // Set client options
//     clientOptions := options.Client().ApplyURI(uri)

//     // Connect to MongoDB
//     client, err := mongo.Connect(context.TODO(), clientOptions)
//     if err != nil {
//         log.Fatal(err)
//     }

//     // Check the connection
//     err = client.Ping(context.TODO(), nil)
//     if err != nil {
//         log.Fatal(err)
//     }

//     fmt.Println("Connected to MongoDB!")

//     // Get a handle for your collection
//     collection := client.Database("testdb").Collection("testcollection")

//     // Create a document to insert
//     document := bson.D{
//         {Key: "name", Value: "Alice"},
//         {Key: "age", Value: 25},
//         {Key: "city", Value: "New York"},
//     }

//     // Insert the document
//     insertResult, err := collection.InsertOne(context.TODO(), document)
//     if err != nil {
//         log.Fatal(err)
//     }

//     fmt.Println("Inserted a single document: ", insertResult.InsertedID)

//     // Find a single document
//     var result bson.M
//     filter := bson.D{{Key: "name", Value: "Alice"}}
//     err = collection.FindOne(context.TODO(), filter).Decode(&result)
//     if err != nil {
//         log.Fatal(err)
//     }

//     // Print the result
//     fmt.Println("Found a single document: ", result)

//     // Disconnect from MongoDB
//     err = client.Disconnect(context.TODO())
//     if err != nil {
//         log.Fatal(err)
//     }

//     fmt.Println("Connection to MongoDB closed.")
// }
