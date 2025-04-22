// Firebase Paths
var dataTempSPath = 'sensor_data/skintemp';
var dataTempAPath = 'sensor_data/ambtemp';
var dataHumPath = 'sensor_data/humidity';

// Variables to store current readings
let tempSReading, tempAReading, humReading;

// Firebase refs
const databaseTempS = database.ref(dataTempSPath);
const databaseTempA = database.ref(dataTempAPath);
const databaseHum = database.ref(dataHumPath);

// Global Variables
let minTemp = 35; 
let lastMinUpdateTime = 0;
const MIN_UPDATE_INTERVAL = 10000;
let restingHR = 70;

// Calculate mPSI
function calculateMPSI(currentTemp, currentHR) {
    const age = parseInt(document.getElementById('age').value) || 22;
    const estimatedHR = estimateHR(currentHR, age);
    currentT = convertF(currentTemp);
    minT = convertF(minTemp);
    const mPSI_temp = 5 * (currentT - minT) / ((39.5 - .25*getAgeScore()) - minT);
    const mPSI_HR = 5 * (estimatedHR - currentHR) / ((220-age) - currentHR);
    const raw_mPSI = mPSI_temp + mPSI_HR;
    return ((9*raw_mPSI)/8)+1
}

// Estimate HR based on age
function estimateHR(avgHR, age) {
    let intensity = 0;
    const workType = parseInt(document.getElementById('workType').value);
    console.log("workType", workType);
    if ([1].includes(workType)) {
       intensity = 0.30;
    } else if ([2].includes(workType)) {
       intensity = 0.35;
    } else if ([3].includes(workType)) {
       intensity = 0.40;
    } else if ([4].includes(workType)) {
       intensity = 0.43;
    } else if ([5].includes(workType)) {
       intensity = 0.48;
    } else if ([6,7].includes(workType)) {
       intensity = 0.5;
    } else if ([8].includes(workType)) {
       intensity = 0.6;
    }
    return avgHR + (Math.floor((220 - age)-avgHR) * intensity);
}

function convertF(temp){
    return (temp-32)* 5/9; 
}

// Get numeric age score
function getAgeScore() {
    const age = parseInt(document.getElementById('age').value);
    if ([22, 27, 32, 37].includes(age)) return 1;
    if ([42, 47].includes(age)) return 2;
    if ([52, 57].includes(age)) return 3;
    if ([62, 67].includes(age)) return 4;
    return 5;
}

// Calculate Heat Index
function calculateHI(RH, T) {
    let hI = T;
    if (T >= 80) {
        hI = -42.379 + 2.04901523 * T + 10.14333127 * RH - 0.22475541 * T * RH
            - 6.83783e-3 * T * T - 5.481717e-2 * RH * RH
            + 1.22874e-3 * T * T * RH + 8.5282e-4 * T * RH * RH
            - 1.99e-6 * T * T * RH * RH;

        if (RH < 13 && T <= 112) {
            hI -= ((13 - RH) / 4) * Math.sqrt((17 - Math.abs(T - 95)) * 0.05882);
        } else if (RH > 85 && T <= 87) {
            hI += ((RH - 85) / 10) * ((87 - T) / 5);
        }
    }
    return hI;
}
// Risk level
function getRiskLevel(heatIndex) {
    if (heatIndex < 80) return 1;
    if (heatIndex < 91) return 2;
    if (heatIndex < 103) return 3;
    if (heatIndex < 125) return 4;
    return 5;
}

// Color mappings
function setmPSIColor(value) {
    if (value <= 3) {
        document.getElementById("mPSI_strain").textContent = "Little to No Strain";
        return "green";
    }
    if (value <= 6) {
        document.getElementById("mPSI_strain").textContent = "Moderate Strain";
        return "yellow";
    }
    else {
        document.getElementById("mPSI_strain").textContent = "Extreme Strain";
        return "red";
    }
}

function sethIColor(value) {
    if (value <= 2) {
        document.getElementById("hI_strain").textContent = "Normal Weather";
        return "green";
    }
    if (value <= 4) {
        document.getElementById("hI_strain").textContent = "Moderate Weather";
        return "yellow";
    }
    else {
        document.getElementById("hI_strain").textContent = "Extreme Weather";
        return "red";
    }
}

function setpHRColor(value) {
    if (value <= 2) {
        document.getElementById("pHR_strain").textContent = "No Additional Heat Risk";
        return "green";
    }
    if (value <= 4) {
        document.getElementById("pHR_strain").textContent = "Moderate Additional Heat Risk";
        return "yellow";
    }
    else {
        document.getElementById("pHR_strain").textContent = "Serious Additional Heat Risk";
        return "red";
    }
}

// Update circle colors
function updateColor() {
    const mPSIVal = parseFloat(document.getElementById("mPSI").textContent);
    const HIVal = parseFloat(document.getElementById("heatIndex").textContent);
    const pHRVal = parseFloat(document.getElementById("heatRiskScore").textContent);

    if (!isNaN(mPSIVal)) document.getElementById("mPSICircle").style.backgroundColor = setmPSIColor(mPSIVal);
    if (!isNaN(HIVal)) document.getElementById("heatIndexCircle").style.backgroundColor = sethIColor(HIVal);
    if (!isNaN(pHRVal)) document.getElementById("personalHeatRiskCircle").style.backgroundColor = setpHRColor(pHRVal);
    

}

// determine and set recommendation text based on values 
function setRec(){
    document.getElementById("mPSI_rec").textContent = setmPSIrec(); 
    let [level, message] = sethIrec();
    document.getElementById("hI_recLevel").textContent = level;
    document.getElementById("hI_rec").textContent = message;
    document.getElementById("pHR_rec").textContent = setpHRrec(); 
}

function setmPSIrec(){
    const mPSI = parseFloat(document.getElementById("mPSI").textContent);
    if (4 < mPSI <=5) return "tired but not at risk of heat related injury."
    if (5 < mPSI <=6) return "very tired but not at risk of heat related injury."
    if (6 < mPSI <=7.5) return "very tired, nearly at risk of heat related injury."
    if (7.5 < mPSI <=8.5) return "exhausted, at risk of heat related injury."
    return "exhausted, at extreme risk of heat related injury."
}

function sethIrec() {
    const HI = parseFloat(document.getElementById("heatIndex").textContent);
    
    if (1 < HI <= 2) {
      return ["Caution", "fatigue possible with prolonged exposure."];
    }
    if (2 < HI <= 3) {
      return ["Extreme Caution", "muscle cramps and heat exhaustion possible."];
    }
    if (3 < HI <= 4) {
      return ["Danger", "heat exhaustion likely; heat stroke possible."];
    }
    if (4 < HI <= 5) {
      return ["Extreme Danger", "heat stroke likely."];
    }
    
    return ["Normal", "no special action needed"];
  }  

function setpHRrec(){
    const pHR = parseFloat(document.getElementById("heatRiskScore").textContent);
    if (1 < pHR <= 2) return "slightly increased risk"
    if (2 < pHR <= 3) return "moderately increased risk"
    if (3 < pHR <= 4) return "highly increased risk"
    if (4 < pHR <= 5) return "extremely increased risk"    
}

// Update displayed values
function updateVal(mPSI, HI, fHR) {
    const mPSIElement = document.getElementById("mPSI");
    const HIElement = document.getElementById("heatIndex");
    const fHRElement = document.getElementById("finalHeatScore");

    if (mPSIElement) mPSIElement.textContent = isNaN(mPSI) ? "NaN" : mPSI.toFixed(2);
    if (HIElement) HIElement.textContent = isNaN(HI) ? "NaN" : HI.toFixed(2);
    if (fHRElement) fHRElement.textContent = isNaN(fHR) ? "NaN" : fHR.toFixed(2);
}

// Full final score calculation
function calculatefHR(){
restingHR = parseInt(document.getElementById('restingHR')) || 70;
const mPSI = calculateMPSI(tempSReading, restingHR);
const HI = calculateHI(humReading, tempAReading);
const heatIndex = getRiskLevel(HI);
const personalScore = parseFloat(document.getElementById("heatRiskScore").textContent) || 0;
const finalHeatScore = 0.4 * (mPSI / 10) + 0.4 * (heatIndex / 5) + 0.2 * (personalScore / 5);
return [mPSI, heatIndex, personalScore, finalHeatScore]
}

// display recalculate button
function recalculateButton(){
    const container = document.getElementById("container");
    console.log("container", container);
    const recalculateButton = document.getElementById("recalculate");
    if (container) {
        container.style.display = "none";
        console.log("container1", container.style.display);
      }
    if (recalculateButton) {
    recalculateButton.classList.remove("hidden");
    document.getElementById('calculate').classList.add('hidden');
    }
}

// Personal risk score calculation
function calculatepHR() {
    console.log("calculatePHR called!");
    let weights = {
        clothing: 0.145, age: 0.21, activity: 0.145,
        medications: 0.145, comorbidities: 0.21, sleep: 0.145
    };

    let clothing = parseInt(document.getElementById('clothing').value);
    let activity = parseInt(document.getElementById('activity').value);
    let sleep = parseInt(document.getElementById('sleep').value);
    let ageScore = getAgeScore();

    let comorbidities = 0;
    if (document.getElementById('obesity').checked) comorbidities += 1;
    if (document.getElementById('diabetes').checked) comorbidities += 1;
    if (document.getElementById('highBloodPressure').checked) comorbidities += 1;
    if (document.getElementById('cardiovascularDisease').checked) comorbidities += 2;

    let medications = document.getElementById('medications').value.toLowerCase().split(",");
    let medList = ["furosemide", "hydrochlorothiazide", "acetazolamide", "atenolol", "metoprolol", "propranolol", "amlodipine", "felodipine", "nifedipine", "enalapril", "lisinopril", "ramipril", "valsartan", "losartan", "clopidogrel", "aspirin", "fluoxetine", "sertraline"];
    let medScore = medications.filter(m => medList.includes(m.trim())).length;

    let score = clothing * weights.clothing + ageScore * weights.age + activity * weights.activity +
                Math.min(medScore, 5) * weights.medications + comorbidities * weights.comorbidities +
                sleep * weights.sleep;
    

    // display value 
    document.getElementById('heatRiskScore').textContent = score.toFixed(2);  
}

function onButtonPress(){  
    recalculateButton(); // display recalculate butoton
    calculatepHR(); 
    results = calculatefHR(); // calculate final heat risk 
    mPSI = results[0];
    hI = results[1];
    pHR = results[2];
    fHR = results[3]; 
    updateVal(mPSI, hI,fHR);  //update values
    updateColor();  //update colors
    setRec();  //determine recommendation text
    //display final heat risk score
    document.getElementById('finalHeatScore').classList.remove('hidden');
    document.getElementById('recalculate').classList.remove('hidden');
    displayRec()     //display recommendation
    
}

function displayRec(){
    const mPSIVal = parseFloat(document.getElementById("mPSI").textContent);
    console.log("mPSI:", mPSIVal);
    if (!isNaN(mPSIVal) && mPSIVal >= 4) {
        document.getElementById("recommendations_mPSI").classList.remove("hidden");
    }
    const hIVal = parseFloat(document.getElementById("heatIndex").textContent);
    console.log("Heat Index:", hIVal);
    if (!isNaN(hIVal) && hIVal >= 2) {
        document.getElementById("recommendations_HI").classList.remove("hidden");
    }
    const pHRVal = parseFloat(document.getElementById("heatRiskScore").textContent);
    console.log("Personal Heat Risk:", pHRVal);
    if (!isNaN(pHRVal) && pHRVal >= 2) {
        document.getElementById("recommendations_pHR").classList.remove("hidden");
    }
}

// Reset form
function resetForm() {
    const container = document.getElementById("container");
    const recalculateButton = document.getElementById("recalculate");
    if (container) {
        container.style.display = "block";
      }
    if (recalculateButton) {
    recalculateButton.classList.add("hidden");
    document.getElementById('calculate').classList.remove('hidden');
    }
    document.getElementById('finalHeatScore').classList.add('hidden');
  
    document.getElementById('heatRiskScore').textContent = '';
    document.getElementById("personalHeatRiskCircle").style.backgroundColor = "grey";
    document.getElementById('heatIndex').textContent = '';
    document.getElementById("heatIndexCircle").style.backgroundColor = "grey";
    document.getElementById('mPSI').textContent = '';
    document.getElementById("mPSICircle").style.backgroundColor = "gray";

    document.getElementById("recommendations_mPSI").classList.add('hidden');
    document.getElementById("recommendations_HI").classList.add('hidden');
    document.getElementById("recommendations_pHR").classList.add('hidden');
}

// Firebase listeners
databaseTempS.on('value', (snapshot) => {
    tempSReading = snapshot.val();
});
databaseTempA.on('value', (snapshot) => {
    tempAReading = snapshot.val();
    document.getElementById("reading-tempA").textContent = tempAReading;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});
databaseHum.on('value', (snapshot) => {
    humReading = snapshot.val();
    document.getElementById("reading-humidity").textContent = humReading;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});

// Track the lowest skin temperature for mPSI reference
function updateMinimums() {
    if (tempSReading !== undefined) minTemp = Math.min(minTemp, tempSReading);
    lastMinUpdateTime = Date.now();
}

// Periodically check if we need to update minimums
function updatePage() {
    const now = Date.now();
    if (now - lastMinUpdateTime > MIN_UPDATE_INTERVAL) updateMinimums();
}

console.log('Firebase client app running...');