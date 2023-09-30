'use strict'

//devices
var audioSource = document.querySelector('select#audioSource');
var audioOutput = document.querySelector('select#audioOutput');
var videoSource = document.querySelector('select#videoSource');

 

var videoplay = document.querySelector('video#player');
//var audioplay = document.querySelector('audio#audioplayer');

//div
var divConstraints = document.querySelector('div#constraints');

//record
var recvideo = document.querySelector('video#recplayer');
var btnRecord = document.querySelector('button#record');
var btnPlay = document.querySelector('button#recplay');
var btnDownload = document.querySelector('button#download');

var buffer;
var mediaRecorder;

//socket.io
var btnConnect = document.querySelector('button#connect');
var btnLeave = document.querySelector('button#leave');
var inputRoom = document.querySelector('input#room');

var socket;
var room;

function gotDevices(deviceInfos){

	deviceInfos.forEach(function(deviceinfo){
		var option = document.createElement('option');
		option.text = deviceinfo.label;
		option.value = deviceinfo.deviceId;
	
		switch(deviceinfo.kind){
			case 'audioinput':
				audioSource.appendChild(option);
				break
			case 'audiooutput':
				audioOutput.appendChild(option);
				break
			case 'videoinput':
				videoSource.appendChild(option);
				break
		}
	})
}

//获得视频流的时候的回调
function gotMediaStream(stream){

	var videoTrack = stream.getVideoTracks()[0];
	var videoConstraints = videoTrack.getSettings();
	divConstraints.textContent = JSON.stringify(videoConstraints, null, 2);


	var audioTrack = stream.getAudioTracks()[0];
	var audioConstraints = audioTrack.getSettings();
	divConstraints.textContent+= JSON.stringify(audioConstraints, null, 2);

	window.stream = stream;
	videoplay.srcObject = stream;

	//audioplay.srcObject = stream;
	return navigator.mediaDevices.enumerateDevices();
}
//获得视频流 出错的回调
function handleError(err){
	console.log('getUserMedia error:', err);
}

function start() {

	if(!navigator.mediaDevices ||
		!navigator.mediaDevices.getUserMedia){

		console.log('getUserMedia is not supported!');
		return;

	}else{
		var deviceId = videoSource.value; 
		var constraints = {
			video : {
				width: 640,	
				height: 480,
				frameRate:15,
				facingMode: 'enviroment',
				deviceId : deviceId ? {exact:deviceId} : undefined 
			}, 
			audio : {
				noiseSuppression: true,   //降噪
				echoCancellation: true   //回音消除
			} 
		}

		navigator.mediaDevices.getUserMedia(constraints)
			.then(gotMediaStream)
			.then(gotDevices)
			.catch(handleError);
	}
}

start();

videoSource.onchange = start;



function handleDataAvailable(e){
	if(e && e.data && e.data.size > 0){
	 	buffer.push(e.data);			
	}
}

function startRecord(){
	
	buffer = [];

	var options = {
		mimeType: 'video/webm;codecs=vp8'
	}

	if(!MediaRecorder.isTypeSupported(options.mimeType)){
		console.error(`${options.mimeType} is not supported!`);
		return;	
	}

	try{
		mediaRecorder = new MediaRecorder(window.stream, options);
	}catch(e){
		console.error('Failed to create MediaRecorder:', e);
		return;	
	}

	mediaRecorder.ondataavailable = handleDataAvailable;
	mediaRecorder.start(10);

}

function stopRecord(){
	mediaRecorder.stop();
}

btnRecord.onclick = ()=>{

	if(btnRecord.textContent === 'Start Record'){
		startRecord();	
		btnRecord.textContent = 'Stop Record';
		btnPlay.disabled = true;
		btnDownload.disabled = true;
	}else{
		stopRecord();
		btnRecord.textContent = 'Start Record';
		btnPlay.disabled = false;
		btnDownload.disabled = false;

	}
}

btnPlay.onclick = ()=> {
	var blob = new Blob(buffer, {type: 'video/webm'});
	recvideo.src = window.URL.createObjectURL(blob);
	recvideo.srcObject = null;
	recvideo.controls = true;
	recvideo.play();
}

btnDownload.onclick = ()=> {
	var blob = new Blob(buffer, {type: 'video/webm'});
	var url = window.URL.createObjectURL(blob);
	var a = document.createElement('a');

	a.href = url;
	a.style.display = 'none';
	a.download = 'aaa.webm';
	a.click();
}

function connectServer(){

	socket = io.connect();

	btnLeave.disabled = false;
	btnConnect.disabled = true;

	///////
	socket.on('joined', function(room, id){
		console.log('The User(' + id + ') have joined into ' + room);	
	});

	socket.on('leaved', function(room, id){ 
		console.log('The User(' + id + ') have leaved from ' + room);
	});

	//join room
	room = inputRoom.value;
	if(room !== ''){
		socket.emit('join', room);	
	}

}

btnConnect.onclick = ()=> {
	connectServer();
}

btnLeave.onclick = ()=> {
	if(room !== ''){
		socket.emit('leave', room);
		btnLeave.disabled = true;
		btnConnect.disabled = false;
		socket.disconnect(); 
	}
}

