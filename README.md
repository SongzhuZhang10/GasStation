---
Title:  The Gas Station
Date:   2023-07-04
Authors: Songzhu Zhang

---

# Design Specifications for The Gas Station Simulation

The ‘Chevron’ chain of gas stations is planning to build a new gas station on the University campus. Your task is to provide a real-time simulation of this, the specification of which is given below. The static architecture for the software is given in a separate document [The Gas Station Architecture](The_Gas_Station_Architecture.pdf). You should read this document in conjunction with the architecture document; otherwise, it won’t make much sense.

1.  There are 4 pumps on the forecourt enabling 4 vehicles to be filled up simultaneously.
2.  A display present inside the gas station office shows the real-time status of all pumps.
3.  Drivers have to pay in advance via a credit card before gas is dispensed.
4.  Drivers get to choose the grade of fuel they wish to purchase: There are 4 types based on the Octane rating of the fuel (e.g. 87, 91 etc), the higher the rating, the more the fuel costs.
5.  For each customer, you will have to **simulate**:
	a. Arrival at the pump (you may have to wait in line)
	b. Swiping a credit card.
	c. Removing the gas hose from the pump.
	d. Selecting a grade of fuel.
	e. Dispensing fuel.
	f. Returning the hose to the pump.
	g. Driving away.
6.  Once the customer has swiped their credit card, removed the hose from the pump and selected the grade of fuel. The gas station attendant signals to that pump that it can begin dispensing fuel at a fixed rate of 0.5 liter per second.
7.  As fuel is dispensed by each pump, the display inside the gas station office is updated in real-time so that the operator can see exactly what is happening at all four pumps. The pump itself also displays the cost per liter of each grade of fuel, the grade of fuel selected by the customer and also how much fuel has been dispensed and the running cost of the purchase in real time.
8.  A facility to change the cost of each grade of fuel should be provided to the gas station attendant computer.
9.  The gas station has a relatively small tank which only holds 2000 litres of fuel (500 litres for each of the 4 grades). So, a facility to restock the gas station should be provided. The stock level of fuel remaining in the tank should be displayed on the attendants display in the office and should flash in <span  style="color: red;">**RED**</span> when it gets below 200 litres. At this point, no new transactions can be started until the gas station has been refilled by delivery truck, but customers in the process of refuelling are allowed to complete their transaction (obviously they can only consume, at most the remainder of the fuel in the gas station tank).
10.  Customer cars are to be simulated by ‘**active class**’ objects which are created **randomly** during the simulation and are ‘**programmed**’ to try to consume a random amount (70 litres max) and grade of gas when they arrive at their pre-programmed pump (_they may have to wait in line_). Each customer will also be programmed with a random name and credit card number to complete transactions.
11.  Each pump is simulated via an "**Active Class object**". These will not write anything to the DOS screen. Instead, they write to data pools in a producer/consumer arrangement. The Attendants computer (see below) is responsible for displaying all output to the screen.  
12.  The customer will, as stated previously, be an active object (i.e., a thread running inside a class) and the communication between customer and pump will be via a pipeline rather than any kind of keyboard. In addition, each pump communicates with the gas station attendant’s computer using a datapool in a producer/consumer arrangement. Finally, each pump has access to a data pool which stores the quantity of fuel remaining in the tank; this allows the pump to deduct fuel from the tank in real-time as it is being dispensed.

The attendant’s computer in the office is also simulated as a ‘**Process**’ which has access to the datapool representing the fuel in the tank (so that it can be refilled) and of course the four pumps discussed earlier (via the pipelines) so that the gas station attendant can control the price of fuel and enable/disable the pumps. The attendant’s computer should also record each transaction performed at each of the four pumps, which should include

* Time of purchase
* Credit card number and name of purchaser
* Grade of fuel selected
* Quantity of fuel dispensed.

The history of all transactions for that day can be displayed on demand.

The gas station attendant is ‘you’ controlling the office computer using commands entered at your PCs keyboard.

Any protocol or commands required by your simulation can be chosen by you.

**An additional 20%** will be awarded for what the assessor considers to be bells and whistles features, such as the clever use of say colour graphics (see online notes on cursor control under Win32 and colour in rt.h/rt.cpp files, e.g. MOVE_CURSOR(X,Y) function), or any other innovative features that you can come up with that are relevant to the gas station simulation, such as more than 4 pumps and/or showing in real-time the list of pending customers at a pump. Some bells and whistles features will obviously attract more marks than others; colour for example is not too difficult (see rt files for examples), while code to deal with more than 4 pumps is more interesting and worth more marks.
