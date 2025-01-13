	async function fetchData(url) {
  try {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.json(); // Assuming JSON response
    return data;
  } catch (error) {
    console.error("Error fetching data:", error);
    return null; // Or handle the error as needed
  }
}

let intervalId; // To store the interval ID

function startUpdates() {
  intervalId = setInterval(async () => {
    const data1 = await fetchData("https://api.coinpaprika.com/v1/tickers/xfg-fango");

    if (data1) {
      const num1 = price;

      if (typeof num1 === 'number') {
        const mktcphero = num1 * 7672289;
        displayResult(mktcphero); // Update the display
      } else {
        console.error("API responses did not contain numbers.");
      }
    }
  }, 10000); // Update every 10000ms (10 second) - Adjust as needed
}

function stopUpdates() {
  clearInterval(intervalId);
}

function displayResult(mktcphero) {
  const resultElement = document.getElementById("mktcphero"); // Get the element
  if (resultElement) {
    resultElement.textContent = result;
  }
}
