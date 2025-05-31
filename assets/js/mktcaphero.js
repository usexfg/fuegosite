   async function fetchData(url) {
      try {
        const response = await fetch(url);
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        return data;
      } catch (error) {
        console.error("Error fetching data:", error);
        return null;
      }
    }

    function displayResult(result) {
      const resultElement = document.getElementById("result");
      if (resultElement) {
        resultElement.textContent = `${result.toLocaleString('en-US', { style: 'currency', currency: 'USD', maximumFractionDigits: 0 })}`;
      }
    }

    let intervalId;

async function updateValues() {
  const fuegoUrl = "https://graphsv2.coinpaprika.com/currency/data/xfg-fuego/30d/?quote=usd";
  const fuegoData = await fetchData(fuegoUrl);

  if (fuegoData && fuegoData.length > 0) {
    try {
      let mostRecentTimestamp = 0;
      let mostRecentPrice = null;

      // Iterate through the price data to find the most recent entry
      for (const priceEntry of fuegoData[0].price) {
        const timestamp = priceEntry[0];
        const price = priceEntry[1];

        if (timestamp > mostRecentTimestamp) {
          mostRecentTimestamp = timestamp;
          mostRecentPrice = price;
        }
      }

      if (mostRecentPrice !== null) {
        const fixedRate = 8000008;
        const product = mostRecentPrice * fixedRate;
        displayResult(product);
      } else {
            console.error("API response did not contain a valid 'price' property.");
            console.log("Fuego Data:", fuegoData);
          }
        } catch (error) {
          console.error("Error processing API data:", error);
        }
      } else {
        console.error("No data received or empty array from API");
        console.log("Fuego Data:", fuegoData);
      }
    }

    function startUpdates() {
      updateValues();
      intervalId = setInterval(updateValues, 300000);
    }

    startUpdates();
