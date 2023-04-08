﻿
#pragma once
#include <cstdint>
#include <string_view>
#include <format>
#include "binary_phase.h"


namespace Q {

	/// @brief Representation of a Pauli operator. 
	struct Pauli {
	public:
		using Bitstring = uint64_t;

		constexpr Pauli() = default;

		/// @brief Creates an identity Pauli operator of length n
		/// @param n Number of qubits
		explicit constexpr Pauli(int n) : n(n) {};

		/// @brief Create a Pauli operator from a string, e.g. XIIXZ, -XYYYX, -iZZ, iXIX
		explicit(false) Pauli(std::string_view pauliString);

		/// @brief Create A Pauli operator with just one X at the specified position, e.g. IIXIII
		static constexpr Pauli SingleX(int n, int qubit);
		/// @brief Create A Pauli operator with just one Z at the specified position, e.g. IIZIII
		static constexpr Pauli SingleZ(int n, int qubit);

		constexpr int numQubits() const { return n; };

		/// @brief Check if operator at given qubit has an x component
		constexpr uint64_t x(int qubit) const;
		/// @brief Check if operator at given qubit has a z component
		constexpr uint64_t z(int qubit) const;

		/// @brief Set x component at given qubit position. The behaviour is unspecified if value is not 0 or 1
		constexpr void setX(int qubit, int value);
		/// @brief Set z component at given qubit position. The behaviour is unspecified if value is not 0 or 1
		constexpr void setZ(int qubit, int value);


		/// @brief Get phase of the operator when XZ is represented as -iY
		constexpr BinaryPhase getPhase() const { return phase - getYPhase(); }

		/// @brief Get phase of the operator when Y is represented as iXZ
		constexpr BinaryPhase getXZPhase() const { return phase; }

		constexpr void increasePhase(int phaseInc) { phase += phaseInc; }
		constexpr void decreasePhase(int phaseDec) { phase -= phaseDec; }

		/// @brief Get the operators Pauli weight (i.e. the number of non-identity single-qubit Paulis)
		constexpr int pauliWeight() const;
		/// @brief Get the number of identity single-qubit Paulis in the operator
		constexpr int identityCount() const;

		/// @brief Get the X components of the Pauli as binary string, e.g. XYZI -> 1100
		constexpr Bitstring getXString() const;
		/// @brief Get the Z components of the Pauli as binary string, e.g. XYZI -> 0110
		constexpr Bitstring getZString() const;

		/// @brief Get a binary string with 1 for each identity, e.g. XYZI -> 0001
		constexpr Bitstring getIdentityString() const;

		std::string toString() const;

		constexpr friend bool operator==(const Pauli& a, const Pauli& b) = default;

		/// @brief Commutator of two Pauli operators. The result is in binary form, 0 if 
		///        p1 and p2 commute, 1 if they anticommute. 
		constexpr friend int commutator(const Pauli& p1, const Pauli& p2);

	private:
		constexpr void fromStringOperator(const std::string_view& str);

		// Get the phase that is accumulated by representing Y as iXZ
		constexpr BinaryPhase getYPhase() const { return { std::popcount(r & s) }; }

		Bitstring r{};
		Bitstring s{};
		int n{ 1 };
		BinaryPhase phase;
	};





	Pauli::Pauli(std::string_view pauliString) {
		if (pauliString.starts_with('i')) {
			phase += 1;
			fromStringOperator(pauliString.substr(1));
		}
		else if (pauliString.starts_with("-i")) {
			phase += 3;
			fromStringOperator(pauliString.substr(2));
		}
		else if (pauliString.starts_with('-')) {
			phase += 2;
			fromStringOperator(pauliString.substr(1));
		}
		else {
			fromStringOperator(pauliString);
		}
		phase += getYPhase();
	}

	constexpr Pauli Pauli::SingleX(int n, int qubit) {
		Pauli pauli{ n };
		pauli.setX(qubit, 1);
		return pauli;
	}

	constexpr Pauli Pauli::SingleZ(int n, int qubit) {
		Pauli pauli{ n };
		pauli.setZ(qubit, 1);
		return pauli;
	}


	constexpr uint64_t Pauli::x(int qubit) const { return (r >> qubit) & 1ULL; }

	constexpr uint64_t Pauli::z(int qubit) const { return (s >> qubit) & 1ULL; }

	constexpr void Pauli::setX(int qubit, int value) { r ^= (-value ^ r) & (1ULL << qubit); }

	constexpr void Pauli::setZ(int qubit, int value) { s ^= (-value ^ r) & (1ULL << qubit); }

	/// @brief Get phase of the operator like when XZ is represented as -iY

	constexpr int Pauli::pauliWeight() const { return std::popcount(r | s); }

	constexpr int Pauli::identityCount() const { return n - pauliWeight(); }

	constexpr Pauli::Bitstring Pauli::getIdentityString() const { return ~(r | s); }

	constexpr Pauli::Bitstring Pauli::getXString() const { return r; }

	constexpr Pauli::Bitstring Pauli::getZString() const { return s; }

	std::string Pauli::toString() const {
		constexpr std::array<char, 4> c{ 'I','X','Z','Y' };
		std::string str;
		str.reserve(n);
		for (int i = 0; i < n; ++i) str += c[x(i) + z(i) * 2];
		return str;
	}


	constexpr void Pauli::fromStringOperator(const std::string_view& str) {
		n = static_cast<int>(str.length());
		int i{};
		for (char c : str) {
			switch (c) {
			case 'I': break;
			case 'X': r |= (1ULL << i); break;
			case 'Y': r |= (1ULL << i); s |= (1ULL << i); break;
			case 'Z': s |= (1ULL << i); break;
			default: break;
			}
			++i;
		}
	}

	/// @brief Commutator of two Pauli operators. The result is in binary form, 0 if 
///        p1 and p2 commute, 1 if they anticommute. 
	constexpr int Q::commutator(const Pauli& p1, const Pauli& p2) { return std::popcount((p1.r & p2.s) ^ (p2.r & p1.s)) & 1; }

}


template<class CharT>
struct std::formatter<Q::Pauli, CharT> : std::formatter<std::string_view, CharT> {
	template<class FormatContext>
	auto format(const Q::Pauli& op, FormatContext& fc) const {
		if (const auto phase = op.getPhase(); phase != Q::BinaryPhase{ 0 }) {
			std::format_to(fc.out(), "{}", phase.toString());
		}
		return std::format_to(fc.out(), "{}", op.toString());
	}
};