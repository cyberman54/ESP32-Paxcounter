/* LoRaWAN Timeserver

VERSION: 1.4

construct 6 byte timesync_answer from gateway timestamp and node's time_sync_req

byte    meaning
1       sequence number (taken from node's time_sync_req)
2..5    current second (from GPS epoch starting 1980)
6       1/250ths fractions of current second

*/

function timecompare(a, b) {
  
  const timeA = a.time;
  const timeB = b.time;

  let comparison = 0;
  if (timeA > timeB) {
    comparison = 1;
  } else if (timeA < timeB) {
    comparison = -1;
  }
  return comparison;
}

let confidence = 1000; // max millisecond diff gateway time to server time
let TIME_SYNC_END_FLAG = 255;

// guess if we have received a valid time_sync_req command
if (msg.payload.payload_raw.length != 1)
  return;

var deviceMsg = { payload: msg.payload.dev_id };
var seqNo = msg.payload.payload_raw[0];
var seqNoMsg = { payload: seqNo };
var gateway_list = msg.payload.metadata.gateways;

// don't answer on TIME_SYNC_END_FLAG
if (seqNo == TIME_SYNC_END_FLAG)
  return;

// filter all gateway timestamps that have milliseconds part (which we assume have a ".")
var gateways = gateway_list.filter(function (element) {
  return (element.time.includes("."));
});

var gateway_time = gateways.map(gw => {
    return {
      time: new Date(gw.time),
      eui: gw.gtw_id,
      }
  });
var server_time = new Date(msg.payload.metadata.time);

// validate all gateway timestamps against lorawan server_time (which is assumed to be recent)
var gw_timestamps = gateway_time.filter(function (element) {
  return ((element.time > (server_time - confidence) && element.time <= server_time));
});

// if no timestamp left, we have no valid one and exit
if (gw_timestamps.length === 0) {
    var notavailMsg = { payload: "n/a" };
    var notimeMsg = { payload: 0xff };    
    var buf2 = Buffer.alloc(1);
    msg.payload = new Buffer(buf2.fill(0xff));
    msg.port = 9; // Paxcounter TIMEPORT
    return [notavailMsg, notavailMsg, deviceMsg, seqNoMsg, msg];}

// sort time array in ascending order to find most recent timestamp for time answer
gw_timestamps.sort(timecompare);

var timestamp = gw_timestamps[0].time;
var eui = gw_timestamps[0].eui;
var offset = server_time - timestamp;

var seconds = Math.floor(timestamp/1000);
var fractions = (timestamp % 1000) / 4;

let buf = new ArrayBuffer(6);
new DataView(buf).setUint8(0, seqNo);
new DataView(buf).setUint32(1, seconds);
new DataView(buf).setUint8(5, fractions);

msg.payload = new Buffer(new Uint8Array(buf));
msg.port = 9; // Paxcounter TIMEPORT
var euiMsg = { payload: eui };
var offsetMsg = { payload: offset };

return [euiMsg, offsetMsg, deviceMsg, seqNoMsg, msg];