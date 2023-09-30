'use strict'
var audioSource  = document.querySelector("select#audioSource");
var audioOutput  = document.querySelector("select#audioOutput");
var videoSource  = document.querySelector("select#videoSource");

if(!navigator.mediaDevices ||
	!navigator.mediaDevices.enumerateDevices){
	console.log('enumerateDevices is not supported!');
}else {
	navigator.mediaDevices.enumerateDevices()
		.then(gotDevices)
		.catch(handleError);
}

function gotDevices(deviceInfos){
	deviceInfos.forEach( function(deviceInfo){
		//2个设备的groupID相同，说明是同一个物理设备

		console.log(deviceInfo.kind + ": label = " 
				+ deviceInfo.label + ": id = "
				+ deviceInfo.deviceId + ": groupId = "
				+ deviceInfo.groupId);	
		
		var option = document.createElement('option');
		option.text = deviceInfo.label;
		option.value = deviceInfo.deviceId;

		switch(deviceInfo.kind){
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
	});

}

function handleError(err){
	console.log(err.name + " : " + err.message);
}
