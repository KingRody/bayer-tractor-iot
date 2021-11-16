import boto3
import json

client = boto3.client('iot-data', region_name='eu-central-1')

labeldata = json.dumps(
{
  "product": "Diflexx",
  "recommendation": {
  "sprayer": {
    "height": 0,
    "width": -1
  },
  "buffer": {
    "distance": 2
  },
  }
})

def lambda_handler(event, context):
    response = client.publish(
        topic='tractor/crop',
        qos=1,
        payload=labeldata
    )