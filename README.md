# smart-grid

A Real-Time Framework for Privacy-Preserving Smart Grid Analytics using Levelled Homomorphic Encryption via CKKS


System Architecture
1. The Smart Meters (Clients) : Encrypted data with levelled HE via CKKS sent to server every intervals imitating a real world scenario
2. The Analytic Server (Server) : Server performing HE analytics on the encrypted data without decrypting it.
3. Logger : Performance Analyzer