import numpy as np


class SdPrior:
    """A prior distribution for a standard deviation 'sigma'.  This prior assumes
    that 1/sigma**2 ~ Gamma(a, b), where a = df/2 and b = ss/2.  Here 'df' is
    the 'sample_size' and ss is the "sum of squares" equal to the sample size
    times 'sigma_guess'**2.

    This prior allows an upper limit on the support of sigma, which is infinite
    by default.

    """

    def __init__(self, sigma_guess, sample_size=.01, initial_value=None,
                 fixed=False, upper_limit=np.inf):
        """Create an SdPrior.

        Args:
          sigma_guess:  Guess at the value of the standard deviation.
          sample_size: Number of observations worth of information with which
            to weight the guess.
          initial_value: The initial value to be used in an MCMC chain.  This
            is not always respected.  The default value is sigma_guess.
          fixed: Flag indicating whether the parameter should be held fixed in
            an MCMC algorithm.  This is mainly for debugging and is not always
            respected.
          upper_limit: Upper limit on the value of 'sigma'.

        """
        self.sigma_guess = float(sigma_guess)
        self.sample_size = float(sample_size)
        if initial_value is None:
            initial_value = sigma_guess
        self.initial_value = float(initial_value)
        self.fixed = bool(fixed)
        self.upper_limit = float(upper_limit)

    @property
    def sum_of_squares(self):
        return self.sigma_guess**2 * self.sample_size
