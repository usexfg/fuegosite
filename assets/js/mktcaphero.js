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
        resultElement.textContent = `${result.toLocaleString('en-US', { style: 'currency', currency: 'USD' })}`;
      }
    }

    let intervalId;

    async function updateValues() {
      const fuegoUrl = "https://graphsv2.coinpaprika.com/currency/data/xfg-fango/7d/?quote=usd";
      const fuegoData = await fetchData(fuegoUrl);

      if (fuegoData) {
        try {
          const closingPrice = fuegoData.data[0].price_high; // Access closing price using updated API structure
          const fixedRate = 8000008;

          if (typeof closingPrice === 'number') {
            const product = closingPrice * fixedRate;
            displayResult(closingPrice, product);
          } else {
            console.error("API response did not contain a valid 'price_high' property.");
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
      // Call updateValues() immediately on page load
      updateValues();

      intervalId = setInterval(updateValues, 30000); // Set the interval for subsequent updates
    }

    startUpdates(); // Start updates when the page loads
