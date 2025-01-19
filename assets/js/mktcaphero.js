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
        resultElement.textContent = "$" + `${result.toLocaleString('en-US', { style: 'currency', currency: 'USD' })}`;
      }
    }

    let intervalId;

    function startUpdates() {
      intervalId = setInterval(async () => {
        // CoinPaprika API for Fuego (XFG) closing price
        const fuegoUrl = "https://graphsv2.coinpaprika.com/currency/data/xfg-fango/7d/?quote=usd";
        const fuegoData = await fetchData(fuegoUrl);

        if (fuegoData) {
          try {
            // Extract the closing price
            const closingPrice = fuegoData[0].price_high;

            // xfg total supply
            const fixedRate = 8000008;

            if (typeof closingPrice === 'number') {
              const product = closingPrice * fixedRate;
              displayResult(product);
            } else {
              console.error("API response did not contain 'close' property.");
              console.log("Fuego Data:", fuegoData);
            }
          } catch (error) {
            console.error("Error processing API data:", error);
          }
        }
      }, 30000); // Update every 30 seconds 
    }

    startUpdates(); // Start updates when the page loads
