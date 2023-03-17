const char index_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE HTML>
	<html>
	<head>
		<meta charset="utf-8">
		<title>DoorBell Configuration</title>
		<meta name="viewport" content="width=device-width, initial-scale=1">
	</head>
	<style>
		* { margin: 0; }
		main{ background-color: aliceblue; }

		html, body, main, .container { height: 100%%; }
		h1, footer{ text-align: center; }
		main, .flexForm, .flexFormContainer, .wrapContent, .flexForm form { display: flex; }
		main, .flexFormContainer, .flexForm { flex-direction: column; }
		main, .flexForm form{ justify-content: space-between; }
		.wrapContent.flex > *, .flexFormContainer > * { white-space: nowrap; }

		h1 {
			background-color: rgb(95, 95, 95);
			color: rgb(214, 214, 214);
			padding: 2vh 1.25vw;
		}
		p {
			padding: 0.5em 0.5vw;
		}
		hr {
			margin: 1em .4em;
		}
		hr.subHr{
			margin: .4em .2em;
		}
		footer {
			background-color: rgb(36, 36, 36);
			color: rgb(214, 214, 214);
		}
		input[type=text]{
			border: none;
			border-bottom: 1px rgb(59, 59, 59) solid;
			border-radius: 0.5em;
		}
		.container {
			padding: 1.2em;
			overflow: auto;
		}
		.flexFormContainer{
			align-items: center;
			margin-bottom: 2.5em;
			min-width: 50%%;
		}
		.flexFormContainer > * {
			width: inherit;
		}
		.flexForm{
			justify-content: space-evenly;
			max-width: 20em;
		}
		.flexForm form > input {
			margin: 0 1.3em;
		}
		.flexForm form > input:last-child{ margin-right: 0; }
		.flexForm form > input:first-child{ margin-left: 0; }

		.wrapContent{
			flex-wrap: wrap;
			justify-content: space-around;
		}
		.wrapContent.flex > * {
			flex: 1;
		}
        .wrapContent.flex {
            overflow-x: auto;
        }
        @media only screen and (orientation:portrait) {
            .wrapContent.flex > *{
                min-width: 95%%;
            }
        }
	</style>
	<body>
		<main>
			<h1>DoorBell Configuration</h1>

			<div class="container">
				<div class="wrapContent flex">
					<p> WiFi Network Name : <span id="WiFi_Name">%inputString_N_WiFi%</span> </p>
					<p> WiFi Status : <span id="WLAN_Connect">%inputString_WLAN_Connect%</span> </p>
					<p> WiFi Level : <span id="RSSI">%inputString_RSSI%</span> dBm</p>
					<p> IP Address : <span id="WLAN_IP">%inputString_WLAN_IP%</span> </p>
					<p> Button Test : <span id="BUTTON">%inputString_BUTTON%</span> </p>
				</div>
		
				<hr>
				
				<div class="wrapContent">
					<div class="flexFormContainer">
						<div class="flexForm">
							<form action="/get" target="hidden-form">
								SimplePush ID 1
								<input type="hidden" name="Test1">
								<input type="submit" value="Test ID 1">
							</form>

							<hr class="subHr">

							<form action="/get" target="hidden-form">
								<input type="text" value='%inputString_ID1_SPSH%' name="SimplePush ID 1">
								<input type="submit" value="Ok" onclick="submitMessage()">
							</form>
						
						</div>
					
						<p> Last Status 1 : <span id="NOTIF1">%inputString_NOTIF1%</span> </p>
					</div>
				
					<div class="flexFormContainer">
						<div class="flexForm">
							<form action="/get" target="hidden-form"> 
								SimplePush ID 2

								<input type="hidden" name="Test2">
								<input type="submit" value="Test ID 2"> 
							</form>

							<hr class="subHr">

							<form action="/get" target="hidden-form">
								<input type="text" value='%inputString_ID2_SPSH%' name="SimplePush ID 2">
								<input type="submit" value="Ok" onclick="submitMessage()">
							</form>
						</div>
					
						<p> Last Status 2 : <span id="NOTIF2">%inputString_NOTIF2%</span> </p>
					</div>
				</div>

				<div style="text-align: center;">
					<form action="/get" target="hidden-form" style="margin-bottom: 2em;"> 
						<input type="hidden" name="RESET">
						<input type="submit" value="RESET" onclick="submitMessage()"> 
					</form>
					<a href="/update">Update</a>
				</div>
				
				<iframe style="display:none" name="hidden-form"></iframe>
			</div>
			<footer><p>V1.7 FP/LP 12/2022</p></footer>
		</main>
	</body>
	<!-------------------------JavaScript------------------------->
	<script>
		function submitMessage() {
			alert("Saved"); 
			setTimeout(function(){ document.location.reload(false); }, 500);   
		}

		const xmlHttpRequestFunction = function(route){
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) document.getElementById(route).innerHTML = this.responseText; };
			xhttp.open("GET", `/${route}`, true);
			xhttp.send();
		}
        setInterval(function() { xmlHttpRequestFunction("WiFi_Name"); }, 5000);
		setInterval(function() { xmlHttpRequestFunction("WLAN_Connect"); }, 2000);
		setInterval(function() { xmlHttpRequestFunction("RSSI"); }, 500);
		setInterval(function() { xmlHttpRequestFunction("WLAN_IP"); }, 5000);
		setInterval(function() { xmlHttpRequestFunction("NOTIF1"); }, 1000);
		setInterval(function() { xmlHttpRequestFunction("NOTIF2"); }, 1000);
		setInterval(function() { xmlHttpRequestFunction("BUTTON"); }, 1000);
	</script>
	</html>
)rawliteral";