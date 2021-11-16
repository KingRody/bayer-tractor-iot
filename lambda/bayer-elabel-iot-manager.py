import json
import boto3
from boto3.dynamodb.conditions import Key
import uuid
import datetime
import time

iotClient = boto3.client('iot-data', region_name='eu-central-1')
iotTopic = 'tractor/crop'
dynamodb = boto3.resource('dynamodb')
table_device = dynamodb.Table('bayer-elabel-device')
table_usage = dynamodb.Table('bayer-elabel-run')
device_id = "c1226964-3a67-11ec-8d3d-0242ac130003"
product = "DEFAULT"


diflexx_recommendation = {
    "sprayer": {
      "height" : 2, 
      "width": 0
    },
    "buffer":{
      "distance" : 1 
    }
}

mateno_recommendation = {
    "sprayer": {
      "height" : 1, 
      "width": 2
    },
    "buffer":{
      "distance" : 2 
    }
}

decis_recommendation = {
    "sprayer": {
      "height" : 0, 
      "width": 1
    },
    "buffer":{
      "distance" : 3 
    }
}

default_recommendation = {
    "sprayer": {
      "height" : 2, 
      "width": 0
    },
    "buffer":{
      "distance" : 2 
    }
}

def lambda_handler(event, context):
    
    print('Received Event:',event)
    
    #Get Product from Request
    #if event['requestContext']['http']['method']=='POST':
    product = event['queryStringParameters']['product']
    print('Received Product:',product)
    
    table_device.update_item(
        Key={
                'id': device_id,
            },
        UpdateExpression="set active_product = :g",
        ExpressionAttributeValues={
                ':g': product
            },
        ReturnValues="UPDATED_NEW"
        )
    
    id = str(uuid.uuid4())
    ts = time.time()
    dts= datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
    
    #  Get product from Dynameo
    response = table_device.query(KeyConditionExpression=Key('id').eq(device_id))
    print(response)
    
    product = response['Items'][0]['active_product']
    
    #  Map settings
    recommendation=default_recommendation
    
    if product == "DIFLEXX":
        recommendation=diflexx_recommendation
    elif product == "MATENO":
        recommendation=mateno_recommendation
    elif product == "DECIS":
        recommendation=decis_recommendation
    
    #  Build body
    result = {
        "device":"XXX",
        "product":product,
        "label":"XXX",
        "recommendation": recommendation
    }
    
    table_usage.put_item(Item= {'id': id,'device':  device_id, 'product':product, 'time':dts, 'recommendation':recommendation})
    
    #Write in IoT topic
    print('Returned Result:', result)
    iotClient.publish(topic=iotTopic,qos=1,payload=json.dumps(result))
    
    return {
        'statusCode': 200,
        'body': json.dumps(result)
    }
