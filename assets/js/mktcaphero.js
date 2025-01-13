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
        resultElement.textContent = result.toFixed(2); // Display with 2 decimal places
      }
    }

    let intervalId;

    function startUpdates() {
      intervalId = setInterval(async () => {
        // CoinPaprika API for Fuego price in USD
        const fuegoUrl = "https://api.coinpaprika.com/v1/tickers/xfg-fango";

        const fuegoData = await fetchData(fuegoUrl);

        if (fuegoData) {
          try {
            // Extract values based on CoinPaprika structure
            const fuegoPriceUsd = parseFloat(fuegoData.quotes.USD.price); // Parse to float

            // circulating xfg supply
            const fixedRate = 7672290; 

            if (typeof fuegoPriceUsd === 'number') {
              const product = fuegoPriceUsd * fixedRate;
              displayResult(product);
            } else {
              console.error("API response did not contain expected number value.");
              console.log("Fuego Data:", fuegoData);
            }
          } catch (error) {
            console.error("Error processing API data:", error);
          }
        }
      }, 30000); // Update every 30 seconds
    }

    startUpdates(); // Start updates when the page loads
